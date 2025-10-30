#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "neurone_system.h"
#include <glib-2.0/glib.h>
#define RESULTS_FILE "results.txt"
#include <cairo.h>
#include <math.h>
#include "interface_v1.h"
#include "traitement_image.h"
#include <stdio.h>
#include "segmenter.h"
//struct for image info in rotation

typedef struct 
{
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	double angle;
} ImageData; 

void open_resolution_window(GtkWidget *widget, gpointer data);
void open_training_window(GtkWidget *widget, gpointer data);
GtkWidget *box_testing_contain = NULL;
GList *pending_results = NULL; 
GtkWidget *main_window = NULL;
GtkWidget *loading_label = NULL;
GtkWidget *global_loading_window = NULL;
//Struct used for the training section to give the result after learning
typedef struct {
    GtkWidget *entry;
    char *label_text;
} ResultData;

//      Structure used to pass data to the training thread. 
//      Contains all necessary information to train a neural network.
typedef struct {
    int nb_inputs;
    int **input;
    int *output;
    int nb_layer;
    int *nb_neurone;
    gchar *name;
} ThreadData;

//---- save_pending_results_to_file() : 
//      the function saves all the current learning results to a file
void save_pending_results_to_file() {
    FILE *f = fopen(RESULTS_FILE, "w");
    if (!f) return;

    for (GList *l = pending_results; l != NULL; l = l->next) {
        fprintf(f, "%s\n", (char *)l->data);
    }
    fclose(f);
}

//---- save_pending_results_to_file() : 
//      loads previously saved learning results from a file into memory
void load_pending_results_from_file() {
    FILE *f = fopen(RESULTS_FILE, "r");
    if (!f) return;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = '\0';  
        pending_results = g_list_append(pending_results, g_strdup(buffer));
    }
    fclose(f);
}




//-------------------------------
//         Main window 
//-------------------------------
int main (int argc, char *argv[])
{
	//Les widgets de l'interface 
	GtkWidget *window;
	GtkWidget *buttonUpload;
	GtkWidget *buttonTraining;
	GtkWidget *label;
	GtkWidget *box;
	GtkWidget *space;
	GtkCssProvider *css_;
	GtkWidget *hbox;	
	
	gtk_init(&argc, &argv);

    load_pending_results_from_file();


	//Initialize the widgets
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	main_window = window;
	gtk_window_set_title (GTK_WINDOW (window), "SolvLad");
	
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
		
	// Main window
	label = gtk_label_new("Welcome to SolvLab");
	space = gtk_label_new(NULL);
	buttonUpload = gtk_button_new_with_label("Solve your grid");
	buttonTraining = gtk_button_new_with_label("Training section");
	
	
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), box);


	gtk_box_pack_start (GTK_BOX(box), label, TRUE, TRUE, 0);
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_name(label, "title");
	
	
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
	gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 20);

	// Solver button
    	gtk_box_pack_start(GTK_BOX(hbox), buttonUpload, TRUE, TRUE, 0);
	gtk_widget_set_size_request(buttonUpload, 250, 100);
	g_signal_connect(buttonUpload, "clicked", G_CALLBACK(open_resolution_window), NULL);
	gtk_widget_set_name(buttonUpload, "btn-upload");	

	// Training button 
	gtk_box_pack_start(GTK_BOX(hbox), buttonTraining, TRUE, TRUE, 0);
	gtk_widget_set_size_request(buttonTraining, 250, 100); 
        g_signal_connect(buttonTraining, "clicked", G_CALLBACK(open_training_window), NULL);
	gtk_widget_set_name(buttonTraining, "btn-train");

	gtk_box_pack_start(GTK_BOX(box), space, TRUE, TRUE, 0);
	
	// Using CSS to style the interface
	css_ = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css_,
	"window{\n"
	" background-color : #72AD6D;\n}"
	"#title{\n"
	"    font-family: \"Comic Neue\", sans-serif;\n"
        "    font-size: 20px;\n"
        "    font-weight: bold;\n"
        "    color: white;\n}"
	"#btn-upload{\n"
        "    font-family: \"Comic Neue\", sans-serif;\n"
        "    font-size: 15px;\n"
	"    color: black;\n"
        "    background-image:url(\"img/pngtree-letters-cut-out-of-paper-magazine-png-image_233507.png\");\n"
	"    background-size: cover;\n"
        "    background-repeat: no-repeat;\n"
        "    background-color: white;\n}"
	"#btn-train{\n"
        "    font-family: \"Comic Neue\", sans-serif;\n"
        "    font-size: 15px;\n"
	"    color: black;\n"
	"    }", -1, NULL);
	
	GtkStyleContext *context = gtk_widget_get_style_context(window);
    	gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(css_),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    
    	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_),
        GTK_STYLE_PROVIDER_PRIORITY_USER);


	// Open the window with its contents
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}



// ------------------------------
// Nouvelle fenêtre : Résolution
// ------------------------------

// ----------------------SOLVER SECTION----------------------------

// _clicked_button_upload: Handles the "Upload" button click.
// Opens a file chooser dialog to let the user select an image or PDF.
// If a PDF is selected, it opens it using the system default viewer depending on the OS.
// If an image is selected, it loads the image, scales it to 600x400, and displays it in the provided vbox.
// The original upload button is hidden after the image is displayed.
// Proper error handling is included for image loading failures.

void _clicked_button_upload(GtkWidget *widget, gpointer user_data)
{
    	GtkWidget *vbox = GTK_WIDGET(user_data); 
    	GtkWidget *window = gtk_widget_get_toplevel(widget);

	//rotation image
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(window,"key-press-event", G_CALLBACK(_on_key_press),NULL);
	//
    	GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Upload your image",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	char *filenameFileToFree = gtk_file_chooser_get_filename(chooser);
	char *filename = make_grayscale_image(gtk_file_chooser_get_filename(chooser));


        const char *dot = strrchr(filename, '.');
        if (dot && g_ascii_strcasecmp(dot, ".pdf") == 0) {
#ifdef __APPLE__
            char command[512];
            snprintf(command, sizeof(command), "open \"%s\"", filename);
            system(command);
#elif __linux__
            char command[512];
            snprintf(command, sizeof(command), "xdg-open \"%s\"", filename);
            system(command);
#elif _WIN32
            char command[512];
            snprintf(command, sizeof(command), "start \"\" \"%s\"", filename);
            system(command);
#endif
        }
	else 
	{
            GError *error = NULL;
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
            if (pixbuf) 	
	    {
                GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 600, 400, GDK_INTERP_BILINEAR);
                GtkWidget *image = gtk_image_new_from_pixbuf(scaled);
		
		//rotation image : ImageData storage
		ImageData *img_data = g_new0(ImageData, 1);
		img_data->image = image;
		img_data->pixbuf = gdk_pixbuf_copy(pixbuf);
		img_data->angle = 0.0;

		g_object_set_data(G_OBJECT(window), "image_data", img_data);
		//

                gtk_widget_hide(widget); 
                gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
                gtk_widget_show(image);

                g_object_unref(pixbuf);
                g_object_unref(scaled);
            } 
	    else 
	    {
		    g_print("Erreur chargement image : %s\n", error->message);
		    g_error_free(error);
            }
        }

        g_free(filenameFileToFree);
    }

    gtk_widget_destroy(dialog);
    gtk_widget_show_all(window);
}


// The window called by the solver button 
void open_resolution_window(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Résolution d'image");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label = gtk_label_new("Upload your image to solve !");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

    GtkWidget *buttonImage =  gtk_button_new_with_label("Upload");
    gtk_box_pack_start(GTK_BOX(vbox), buttonImage, TRUE, TRUE, 0);
    gtk_widget_set_size_request(buttonImage, 250, 100);
    g_signal_connect(buttonImage, "clicked", G_CALLBACK(_clicked_button_upload), vbox);
    

    gtk_widget_show_all(window);
}

// ----------------------TRAINING SECTION----------------------------


// get_input: Converts a string of input pairs into a dynamically allocated 2D integer array.
// Example input format: "(0,0), (1,0), (1,1)"
// Parameters:
//   inputs      - The input string containing pairs of integers.
//   nb_elmt_out - Pointer to an int where the number of input pairs will be stored.
// Returns:
//   A pointer to a dynamically allocated 2D array of integers [nb_elmt][2].
// Notes:
//   - The function counts the number of ')' characters to determine the number of pairs.
//   - Curly braces and parentheses are replaced by spaces to facilitate parsing.
//   - Each pair of integers is stored in input[i][0] and input[i][1].
//   - Caller is responsible for freeing the allocated memory.

int **get_input(const gchar *inputs, int *nb_elmt_out)
{
    if (!inputs) return NULL;
    int nb_elmt = 0;
    for (int i = 0; inputs[i]; i++) {
        if (inputs[i] == ')')
            nb_elmt++;
    }
    if (nb_elmt_out)
        *nb_elmt_out = nb_elmt; 

    int **input = malloc(nb_elmt * sizeof(int *));
    for (int i = 0; i < nb_elmt; i++)
        input[i] = malloc(2 * sizeof(int));

    char *clean = g_strdup(inputs);
    for (int i = 0; clean[i]; i++) {
        if (clean[i] == '{' || clean[i] == '}' || clean[i] == '(' || clean[i] == ')')
            clean[i] = ' ';
    }

    int x;
    int idx = 0;
    char *token = strtok(clean, ", ");
    while (token && idx < nb_elmt * 2) {
        if (sscanf(token, "%d", &x) == 1) {
            if (idx % 2 == 0)
                input[idx / 2][0] = x;
            else
                input[idx / 2][1] = x;
            idx++;
        }
        token = strtok(NULL, ", ");
    }
    g_free(clean);
    return input;
}


// get_output: Converts a string of output values into a dynamically allocated integer array.
// Example input format: "0, 1, 1, 0"
// Parameters:
//   outputs - The input string containing integer output values.
// Returns:
//   A pointer to a dynamically allocated integer array containing the parsed outputs.
// Notes:
//   - Curly braces and parentheses are replaced with spaces to facilitate parsing.
//   - The array size is currently fixed to 100 elements; adjust if needed.
//   - Caller is responsible for freeing the allocated memory.
//   - The function prints the parsed outputs for debugging purposes.

int *get_output(const gchar *outputs)
{
    if (!outputs) return NULL;

    char *clean = g_strdup(outputs);
    for (int i = 0; clean[i]; i++) {
        if (clean[i] == '{' || clean[i] == '}' || clean[i] == '(' || clean[i] == ')')
            clean[i] = ' ';
    }

    int *array = malloc(100 * sizeof(int));
    int val, count = 0;
    char *token = strtok(clean, ", ");
    while (token) {
        if (sscanf(token, "%d", &val) == 1) {
            array[count++] = val;
        }
        token = strtok(NULL, ", ");
    }

    g_free(clean);
    return array;
}


// get_neurones: Parses a string representing the number of neurons per layer into an integer array.
// Example input format: "2, 3, 1"
// Parameters:
//   nb_neurones - The input string containing the number of neurons for each layer.
// Returns:
//   A pointer to a dynamically allocated integer array containing the parsed neuron counts.
// Notes:
//   - Curly braces and parentheses are replaced with spaces to simplify parsing.
//   - The array size is currently fixed at 100 elements; adjust if needed for larger networks.
//   - Caller is responsible for freeing the allocated memory.

int *get_neurones(const gchar *nb_neurones)
{
    if (!nb_neurones) return NULL;

    char *clean = g_strdup(nb_neurones);

    for (int i = 0; clean[i]; i++) {
        if (clean[i] == '{' || clean[i] == '}' || clean[i] == '(' || clean[i] == ')')
            clean[i] = ' ';
    }

    int *array = malloc(100 * sizeof(int));
    if (!array) return NULL;

    int val, count = 0;
    char *token = strtok(clean, ", ");
    while (token) {
        if (sscanf(token, "%d", &val) == 1) {
            array[count++] = val;
        }
        token = strtok(NULL, ", ");
    }


    g_free(clean);
    return array;
}

//---- on_result: 
//      Uses the neural network to predict the output for the user's input
//      and displays the result in a new window. The input is read from data->entry.
//      The function is called by the on_result_clicked function

void on_result(GtkWidget *button, gpointer user_data)
{  (void)button;
    ResultData *data = (ResultData *)user_data;

    const char *input_text = gtk_entry_get_text(GTK_ENTRY(data->entry));

    int nb_inputs = 0;
    int **input = get_input(input_text, &nb_inputs);
    double input_d[2];
    input_d[0] = (double)input[0][0];
    input_d[1] = (double)input[0][1];
    printf("Avant prediction : input[0] = (%d, %d)\n", input[0][0], input[0][1]);
    double *value = prediction(input_d, data->label_text);
    double val = value[0];
    printf("val : %f\n", val);
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.0f", val);
    GtkWidget *loading_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(loading_window), "Result");
    gtk_window_set_default_size(GTK_WINDOW(loading_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(loading_window), GTK_WIN_POS_CENTER);
    GtkWidget *label = gtk_label_new(buffer);
    gtk_container_add(GTK_CONTAINER(loading_window), label);
    gtk_widget_show_all(loading_window);

    
}

//---- on_result_clicked: 
//      This function is called when a result button is clicked in the testing section.
//      It opens a new window where the user can enter input for the neural network.
//      The "Result" button in the window is connected to on_result(), which computes
//      and displays the neural network's prediction for the entered input.

void on_result_clicked(GtkWidget *button, gpointer user_data) {
    (void)user_data;
    const char *label = gtk_button_get_label(GTK_BUTTON(button));

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Testing Section");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label_input = gtk_label_new("Entry your input");
    GtkWidget *entry_input = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), label_input, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_input, FALSE, FALSE, 0);

    GtkWidget *button_result = gtk_button_new_with_label("Result");
    gtk_widget_set_size_request(button_result, 200, 50);
    gtk_box_pack_start(GTK_BOX(vbox), button_result, FALSE, FALSE, 10);
    gtk_widget_set_halign(button_result, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button_result, GTK_ALIGN_CENTER);

    ResultData *data = g_malloc(sizeof(ResultData));
    data->entry = entry_input;
    data->label_text = g_strdup(label); 

    g_signal_connect(button_result, "clicked", G_CALLBACK(on_result), data);

    gtk_widget_show_all(window);
}

//---- ajouter_resultat: 
//      Adds a new result button to the testing window if it exists.
//      If the testing window is not yet created, the result is stored in 
//      the pending_results list for later use.

void ajouter_resultat(const char *texte_resultat) {
    if (box_testing_contain) {
        GtkWidget *btn = gtk_button_new_with_label(texte_resultat);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_result_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box_testing_contain), btn, FALSE, FALSE, 5);
        gtk_widget_show_all(box_testing_contain);
    } else {
        pending_results = g_list_append(pending_results, g_strdup(texte_resultat));
    }
}

void show_training_finished_dialog() {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    "Entraînement terminé !\n\nAllez dans \"Testing\" pour voir le résultat.");
    gtk_window_set_title(GTK_WINDOW(dialog), "Information");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
//---- ajouter_resultat_idle:
//      Used with g_idle_add to safely add a new result from another thread.
//      It first checks if the result already exists in pending_results to avoid duplicates.
//      The result is saved to file and a button is added to the testing window if available.

gboolean ajouter_resultat_idle(gpointer data) {
    const char *texte_resultat = (const char *)data;

    for (GList *l = pending_results; l != NULL; l = l->next) {
        if (g_strcmp0((const char *)l->data, texte_resultat) == 0) {
            g_free((gpointer)texte_resultat);
            return G_SOURCE_REMOVE;
        }
    }

    pending_results = g_list_append(pending_results, g_strdup(texte_resultat));
    save_pending_results_to_file();  

    if (box_testing_contain) {
        GtkWidget *btn = gtk_button_new_with_label(texte_resultat);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_result_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box_testing_contain), btn, FALSE, FALSE, 5);
        gtk_widget_show_all(box_testing_contain);
    }

    g_free((gpointer)texte_resultat);
    if (loading_label) {
    gtk_label_set_text(GTK_LABEL(loading_label),
        "Entraînement terminé !\nRendez-vous dans \"Testing\" pour voir le résultat.");
}
    return G_SOURCE_REMOVE;
}

//---- thread_function:
//      Function executed in a separate thread to train a neural network.
//      It converts integer inputs/outputs to double, calls the 'principal'
//      function to perform training, and then schedules the result to be
//      added to the GTK interface using g_idle_add. After training, it frees
//      all dynamically allocated memory associated with the ThreadData structure.

gpointer thread_function(gpointer data) {
    ThreadData *td = (ThreadData *)data;
    double **inputs_d = malloc(td->nb_inputs * sizeof(double *));
    for (int i = 0; i < td->nb_inputs; i++) {
        inputs_d[i] = malloc(2 * sizeof(double));
        inputs_d[i][0] = (double) td->input[i][0];
        inputs_d[i][1] = (double) td->input[i][1];
    }

    double *outputs_d = malloc(td->nb_inputs * sizeof(double));
    for (int i = 0; i < td->nb_inputs; i++) 
        outputs_d[i] = (double) td->output[i];

    int *neurones_d = td->nb_neurone;
    
    principal(inputs_d, neurones_d, td->name, outputs_d, td->nb_layer, td->nb_inputs);
    //ajouter_resultat(g_strdup(td->name));
    g_idle_add((GSourceFunc)ajouter_resultat_idle, g_strdup(td->name));

    for (int i = 0; i < td->nb_inputs; i++) free(inputs_d[i]);
    free(inputs_d);
    free(outputs_d);

    for (int i = 0; i < td->nb_inputs; i++) {
        if (td->input[i]) free(td->input[i]);
    }
    free(td->input);
    free(td->output);
    free(td->nb_neurone);

   // cleanup_td:
        if (td->name) g_free(td->name);
        g_free(td);

        fprintf(stderr, ">>> thread_function: exit\n");
        fflush(stderr);
    return NULL;
}

//---- close_loading_window:
//      Callback to close/destroy the temporary loading window after training
//      is finished or after a timeout.
gboolean close_loading_window(gpointer data) {
    GtkWidget *loading_window = GTK_WIDGET(data);
    gtk_widget_destroy(loading_window);
    return FALSE;
}

//---- on_submit:
//      Callback triggered when the user submits the training form.
//      It reads all entries from the form (network name, input/output values,
//      number of layers, neurons per layer), converts them into the proper
//      data structures, shows a loading window, then starts a new thread
//      to train the neural network using 'thread_function'. The loading
//      window will automatically close after 5 seconds.
/*
void on_submit(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget **entries = (GtkWidget **)data;

    const gchar *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const gchar *inputs = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const gchar *outputs = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const gchar *nb_layers = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const gchar *nb_neurones = gtk_entry_get_text(GTK_ENTRY(entries[4]));

    int nb_inputs = 0;
    int **input = get_input(inputs, &nb_inputs);
    int *output = get_output(outputs);
    int nb_layer = atoi(nb_layers);
    int *nb_neurone = get_neurones(nb_neurones);
    

    GtkWidget *loading_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(loading_window), "Chargement...");
    gtk_window_set_default_size(GTK_WINDOW(loading_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(loading_window), GTK_WIN_POS_CENTER);
    loading_label = gtk_label_new("Veuillez patienter...");
    gtk_container_add(GTK_CONTAINER(loading_window), loading_label);
    gtk_widget_show_all(loading_window);


    ThreadData *td = g_malloc(sizeof(ThreadData));
    td->nb_inputs = nb_inputs;
    td->input = input;
    td->output = output;
    td->nb_layer = nb_layer;
    td->nb_neurone = nb_neurone;
    td->name = g_strdup(name); 

   // g_timeout_add_seconds(5, close_loading_window, loading_window);


    GThread *thread = g_thread_new("worker", thread_function, td);
    
    g_thread_unref(thread);

}*/
void on_submit(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget **entries = (GtkWidget **)data;

    const gchar *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const gchar *inputs = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const gchar *outputs = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const gchar *nb_layers = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const gchar *nb_neurones = gtk_entry_get_text(GTK_ENTRY(entries[4]));

    int nb_inputs = 0;
    int **input = get_input(inputs, &nb_inputs);
    int *output = get_output(outputs);
    int nb_layer = atoi(nb_layers);
    int *nb_neurone = get_neurones(nb_neurones);

    // Crée la fenêtre de chargement globale
    GtkWidget *loading_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(loading_window), "Chargement...");
    gtk_window_set_default_size(GTK_WINDOW(loading_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(loading_window), GTK_WIN_POS_CENTER);

    // ✅ Label global pour affichage dynamique
    loading_label = gtk_label_new("⏳ Entraînement en cours...\nVeuillez patienter...");
    gtk_container_add(GTK_CONTAINER(loading_window), loading_label);
    gtk_widget_show_all(loading_window);

    // ✅ Alloue les données du thread
    ThreadData *td = g_malloc(sizeof(ThreadData));
    td->nb_inputs = nb_inputs;
    td->input = input;
    td->output = output;
    td->nb_layer = nb_layer;
    td->nb_neurone = nb_neurone;
    td->name = g_strdup(name);

    // Lance le thread d’entraînement
    GThread *thread = g_thread_new("worker", thread_function, td);
    g_thread_unref(thread);

    // Stocke la fenêtre dans une variable globale (optionnel)
    global_loading_window = loading_window;
}

//---- open_training_network_window:
//      Opens a new window allowing the user to create and configure a neural network for training.
//      The window contains entries for the network name, input values, expected output values,
//      number of layers, and number of neurons per layer. When the "Start learning" button is clicked,
//      the on_submit callback is triggered to start the training in a separate thread.

void open_training_network_window(GtkWidget *widget, gpointer data) {
    (void)data;
    (void)widget;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Training");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);


    GtkWidget *label_name = gtk_label_new("Name your training");
    GtkWidget *entry_name = gtk_entry_new();

    //Values to learn
    GtkWidget *label_input = gtk_label_new("Entry the input");
    GtkWidget *entry_input = gtk_entry_new();

    GtkWidget *label_output = gtk_label_new("Entry the output expected");
    GtkWidget *entry_output = gtk_entry_new();

    //To create the network
    GtkWidget *label_nb_layer = gtk_label_new("Entry the number of layers");
    GtkWidget *entry_nb_layer = gtk_entry_new();

    GtkWidget *label_nb_neurones = gtk_label_new("Entry the number of neurones per layers (ex: 2, 3, 1)");
    GtkWidget *entry_nb_neurones = gtk_entry_new();

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    gtk_grid_attach(GTK_GRID(grid), label_input,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_input,   1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_output, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_output, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_nb_layer,   0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_nb_layer,   1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_nb_neurones,   0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_nb_neurones,   1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_name,   0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_name,   1, 4, 1, 1);

   gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

    GtkWidget *button = gtk_button_new_with_label("Start learning");
    gtk_widget_set_size_request(button, 200, 50);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 10);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

    GtkWidget **entries = g_new(GtkWidget*, 5);
    entries[0] = entry_name;
    entries[1] = entry_input;
    entries[2] = entry_output;
    entries[3] = entry_nb_layer;
    entries[4] = entry_nb_neurones;

    g_signal_connect(button, "clicked", G_CALLBACK(on_submit), entries);

    gtk_widget_show_all(window);
}

//---- open_testing_window:
//      Opens the testing window where the user can see all previously trained networks as buttons.
//      Each button corresponds to a network and can be clicked to open an input field for testing
//      the network. The buttons are dynamically recreated from the saved list 'pending_results'
//      each time the window is opened.

void open_testing_window(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Résolution d'image");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    box_testing_contain = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), box_testing_contain);

    // À chaque ouverture de fenêtre, on recrée les boutons à partir de pending_results
    for (GList *l = pending_results; l != NULL; l = l->next) {
        const char *texte_resultat = (const char *)l->data;
        GtkWidget *btn = gtk_button_new_with_label(texte_resultat);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_result_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box_testing_contain), btn, FALSE, FALSE, 5);
    }

    gtk_widget_show_all(window);
}


//The window called by the training button

void open_training_window(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkCssProvider *css_;
    gtk_window_set_title(GTK_WINDOW(window), "Training section");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *label = gtk_label_new("Training the network or testing");
    GtkWidget *button_Test = gtk_button_new_with_label("Testing");
    GtkWidget *button_Training = gtk_button_new_with_label("Training");
    GtkWidget *space = gtk_label_new(NULL); 

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), box);

    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_name(label, "title");

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, TRUE, 20);

    gtk_widget_set_size_request(button_Test, 250, 100);
    gtk_widget_set_size_request(button_Training, 250, 100);

    gtk_box_pack_start(GTK_BOX(hbox), button_Test, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_Training, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), space, TRUE, TRUE, 0);

    g_signal_connect(button_Test, "clicked", G_CALLBACK(open_testing_window), NULL);
    gtk_widget_set_name(button_Test, "btn-testing");

    g_signal_connect(button_Training, "clicked", G_CALLBACK(open_training_network_window), NULL);
    gtk_widget_set_name(button_Training, "btn-training");

    css_ = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_,
        "window {\n"
        "  background-color: #72AD6D;\n"
        "}\n"
        "#title {\n"
        "  font-family: 'Comic Neue', sans-serif;\n"
        "  font-size: 20px;\n"
        "  font-weight: bold;\n"
        "  color: white;\n"
        "}\n"
        "#btn-testing, #btn-training {\n"
        //"  min-width: 250px;\n"
        //"  min-height: 100px;\n"
        "  font-family: 'Comic Neue', sans-serif;\n"
        "  font-size: 15px;\n"
        "  color: black;\n"
        "  background-color: white;\n"
        "  border-radius: 12px;\n"
        "}\n",
    -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(css_),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_show_all(window);
}




GdkPixbuf* rotate_pixbuf(GdkPixbuf *pixbuf, double angle_degrees)
{
	if (!pixbuf) return NULL;
	
	int w = gdk_pixbuf_get_width(pixbuf);
	int h = gdk_pixbuf_get_height(pixbuf);
	
	double angle = angle_degrees * (M_PI / 180.0);
	
	double cos_a = fabs(cos(angle));
	double sin_a = fabs(sin(angle));
	int new_w = (int)(w * cos_a + h * sin_a);
	int new_h = (int)(w * sin_a + h * cos_a);
	
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_w, new_h);
	cairo_t *c_surface = cairo_create(surface);
	
	cairo_set_source_rgb(c_surface, 1, 1, 1); 
	//cairo_set_source_rgba(c_surface, 0, 0, 0, 0);
	cairo_paint(c_surface);
	
	cairo_translate(c_surface, new_w / 2.0, new_h / 2.0);
	
	cairo_rotate(c_surface, angle);
	
	cairo_translate(c_surface, -w / 2.0, -h / 2.0);
	
    	gdk_cairo_set_source_pixbuf(c_surface, pixbuf, 0, 0);
    	cairo_paint(c_surface);

    	cairo_destroy(c_surface);

    	GdkPixbuf *rotated = gdk_pixbuf_get_from_surface(surface, 0, 0, new_w, new_h);
    	cairo_surface_destroy(surface);

    	return rotated;
}
//input gestion
gboolean _on_key_press(GtkWidget *CurrentWidget, GdkEventKey *KeyEvent, gpointer data_useless)
{
	GtkWidget *window = gtk_widget_get_toplevel(CurrentWidget);
	ImageData *img_data = g_object_get_data(G_OBJECT(window), "image_data");
	if (!img_data || !img_data->pixbuf) 
	{
        	g_print("Aucune image chargée.\n");
        	return FALSE;
    	}

    	double angle_step = 0.0;
	if (KeyEvent->keyval == GDK_KEY_Return)
	{
		GdkPixbuf *rotated_to_save = rotate_pixbuf(img_data->pixbuf, img_data->angle);
	
		gdk_pixbuf_save(/*img_data->pixbuf*/rotated_to_save, "grayscale_rotated.bmp", "bmp", NULL, NULL);
		printf("image saved in grayscale_rotated.bmp");
		char *inputForSegmenter[] = {"segmenter", "grayscale_rotated.bmp"};
		cut_grid(2, inputForSegmenter);
	}
    	if (KeyEvent->keyval == GDK_KEY_r || KeyEvent->keyval == GDK_KEY_R)
        	angle_step = 1;
    	else if (KeyEvent->keyval == GDK_KEY_l || KeyEvent->keyval == GDK_KEY_L)
    	    angle_step = -1;
    	else
    	    return FALSE;

    	img_data->angle += angle_step;

    	GdkPixbuf *rotated = rotate_pixbuf(img_data->pixbuf, img_data->angle);
    	if (rotated) 
	{
    	   	gtk_image_set_from_pixbuf(GTK_IMAGE(img_data->image), rotated);
    	    	g_object_unref(rotated);
   	    	g_print("Image tournée à %.1f degrés\n", img_data->angle);
  	} 
	else 
	{
        	g_print("Erreur lors de la rotation de l'image !\n");
   	}
	
	return TRUE;
}



