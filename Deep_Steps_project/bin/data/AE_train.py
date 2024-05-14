
dataset = np.loadtxt('dataset.csv', delimiter=',')
ae1 = Autoencoder()
ae1.autoencoder.fit(dataset,dataset, epochs, 16)
