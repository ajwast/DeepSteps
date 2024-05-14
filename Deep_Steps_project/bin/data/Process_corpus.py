
bar_length = 1
## Set the desired range for onsets
desired_range = 16 * bar_length

# Check if onsets is greater than the desired range
while len(onsets) > desired_range:
    # Double the bar_length
    bar_length *= 2

    # Recalculate the desired range based on the updated bar_length
    desired_range = 16 * bar_length



#Setting variables for time-base etc.
per_quarter_note = 48
timebase= per_quarter_note*4

#set 16ths and ppqn values based on bar length
num_ppqn = timebase*bar_length
sixteenths_div = 16*bar_length

#Define ppqn and sixteenth note sizes
ppqn_timebase = round(dur / num_ppqn)
sixteenths = round(dur / sixteenths_div)

#Initialise slice arrays
steps = []
ppqn_slices = []

#Get 16th note slice points
for f in range(sixteenths_div):
    out= f * sixteenths
    steps=np.append(steps, out)
#Get PPQN slice points
for f in range(num_ppqn):
    out= f * ppqn_timebase
    ppqn_slices=np.append(ppqn_slices, out)

# Convert onset times to integers using np.round
onsets = np.round(onsets).astype(int)

# Round onset points down to the nearest sixteenth note and disgard double event triggers
onset_points_rounded = []
previous_rounded_onset = None
keep_mask = np.ones(len(onsets), dtype=bool)
for i, onset in enumerate(onsets):
    rounded_onset = int(round(onset / sixteenths))
    # Check if the current rounded onset is the same as the previous one
    if rounded_onset != previous_rounded_onset:
        onset_points_rounded.append(rounded_onset)
        previous_rounded_onset = rounded_onset
    else:
        # If the onset is a duplicate, mark it for removal
        keep_mask[i] = False

# Filter the 'onsets' array to keep only the onsets that haven't been processed
onsets = onsets[keep_mask]

#Round onsets to nearest PPQN timebase point
ppqn_onsets = [int(onset // ppqn_timebase) * ppqn_timebase for onset in onsets]

# One-hot encode the rounded onset points in an array of length 16
num_sixteenths = sixteenths_div
ohe_sixteenths = np.zeros(num_sixteenths)
for onset in onset_points_rounded:
    if onset < num_sixteenths:
        ohe_sixteenths[onset] = 1

#Get distances of substeps from nearest 16th note
substeps = []
quantize_points_rounded_sixteenths = [int(round(ppqn_timebase / sixteenths)) * sixteenths for ppqn_timebase in onsets]
for f, c in zip(ppqn_onsets, quantize_points_rounded_sixteenths):
    m = f - c
    ss = m //ppqn_timebase
    s = (ss - -6) / (6 - -6)
    if s >= 1:
        substeps = np.append(substeps,s)
    else:
        substeps = np.append(substeps,s)


#Match substep values to corresponding event
substeps_full = []
j = 0  # Index for substeps array
for binary_value in ohe_sixteenths:
    if binary_value == 1 and j < len(substeps):
        substeps_full.append(substeps[j])
        j += 1
    else:
        substeps_full.append(0)


ss_arr = np.array(substeps_full)


# Concatenate the two arrays horizontally
#op= np.concatenate((ohe_sixteenths, substeps_full))

if bar_length == 1:
    op= np.concatenate((ohe_sixteenths, substeps_full))
    dataset = np.vstack((dataset,op))
    
else:
    split1 = np.split(ohe_sixteenths,bar_length)
    split2 = np.split(ss_arr,bar_length)
    for i in range(bar_length):
        op = np.concatenate((split1[i], split2[i]))
        dataset = np.vstack((dataset,op))



#if bar_length == 1:
#    dataset = np.append(dataset,op)
#else:
#    splits = np.split(op,bar_length)
#    dataset = np.vstack((dataset,splits))
