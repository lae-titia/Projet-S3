# Projet-S3

pour le img to txt : gcc -Wall -Wextra img_to_txt.c -o img_to_txt $(pkg-config --cflags --libs glib-2.0) -lm
et après ./img_to_txt output/ grille.txt 

Training Section :

The training section is splited in 2 parts :

•	Training : In this section, users provide the required values to train the neural network.
Five fields are available:
•	Input data (e.g., (0,0), (0,1), (1,0), (1,1))
•	Expected outputs (e.g., 0, 1, 1, 0)
•	Number of layers (e.g., 3)
•	Neurons per layer (e.g., 2, 2, 1)
•	Training name to easily locate the model later in the Testing section
Once all values are entered, press Submit to initiate the training.
You will find all your training in the testing section.

•	Testing :  In this section, every tme the network learns something new, the interface automatically creates a button to test that trained model. For example, if you train the network on the XOR function, the sytem saves the learned weights and biaises and generates a button ine the Testing window. This button remains available after closing and reopenoing the interface. To test the model, simply click to the corresponding button and enter the input values you want to evaluate. For instance, if you enter (0,0), a new window will open showing the predicted output from the neural network.
