#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "neurone_system.h"
#include <glib-2.0/glib.h>

void open_resolution_window(GtkWidget *widget, gpointer data);
void open_training_window(GtkWidget *widget, gpointer data);


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
	GdkPixbuf *pixbuf;	
	GtkWidget *hbox;	
	

	//Initialise GTK
	gtk_init(&argc, &argv);

	//Inisialiser les widgets
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "SolvLad");
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
		
	// Fenêtre principale
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

	// Bouton de résolution
    	gtk_box_pack_start(GTK_BOX(hbox), buttonUpload, TRUE, TRUE, 0);
	gtk_widget_set_size_request(buttonUpload, 250, 100);
	g_signal_connect(buttonUpload, "clicked", G_CALLBACK(open_resolution_window), NULL);
	gtk_widget_set_name(buttonUpload, "btn-upload");	

	// Bouton d’apprentissage
	gtk_box_pack_start(GTK_BOX(hbox), buttonTraining, TRUE, TRUE, 0);
	gtk_widget_set_size_request(buttonTraining, 250, 100); 
        g_signal_connect(buttonTraining, "clicked", G_CALLBACK(open_training_window), NULL);
	gtk_widget_set_name(buttonTraining, "btn-train");

	gtk_box_pack_start(GTK_BOX(box), space, TRUE, TRUE, 0);
	
	//Utilisation de CSS pour styliser l'interface
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


	//Ouvrir la fenetre avec son contenu
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}


//FAUT CHANGERRRRRRRRRRRRRRRRRRRRRRRRRR

// ------------------------------
// Nouvelle fenêtre : Résolution
// ------------------------------
void _clicked_button_upload(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *vbox = GTK_WIDGET(user_data); 
    GtkWidget *window = gtk_widget_get_toplevel(widget);

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
        char *filename = gtk_file_chooser_get_filename(chooser);

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
        } else {
            GError *error = NULL;
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
            if (pixbuf) {
                GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 600, 400, GDK_INTERP_BILINEAR);
                GtkWidget *image = gtk_image_new_from_pixbuf(scaled);

                gtk_widget_hide(widget); 
                gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
                gtk_widget_show(image);

                g_object_unref(pixbuf);
                g_object_unref(scaled);
            } else {
                g_print("Erreur chargement image : %s\n", error->message);
                g_error_free(error);
            }
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    gtk_widget_show_all(window);
}



void open_resolution_window(GtkWidget *widget, gpointer data) {
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

int **get_input(const gchar *inputs, int *nb_elmt_out)
{
    if (!inputs) return NULL;

    printf("📥 Texte brut : %s\n", inputs);
    int nb_elmt = 0;
    for (int i = 0; inputs[i]; i++) {
        if (inputs[i] == ')')
            nb_elmt++;
    }

    if (nb_elmt_out)
        *nb_elmt_out = nb_elmt; // ✅ on sauvegarde le nombre d’éléments

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

    printf("\n✅ Tableau final :\n");
    for (int i = 0; i < nb_elmt; i++)
        printf("   Pair %d : (%d, %d)\n", i, input[i][0], input[i][1]);

    g_free(clean);
    return input;
}


int *get_output(const gchar *outputs)
{
    if (!outputs) return NULL;

    printf("📥 Texte brut (outputs) : %s\n", outputs);
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

     printf("🧾 Tableau final : ");
    for (int i = 0; i < count; i++)
        printf("%d", array[i]);
    printf("\n");
    g_free(clean);
    return array;
}


int *get_neurones(const gchar *nb_neurones)
{
    if (!nb_neurones) return NULL;

    printf("📥 Texte brut (nb_neurones) : %s\n", nb_neurones);

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
    printf("🧾 Tableau final des neurones : ");
    for (int i = 0; i < count; i++)
        printf("%d ", array[i]);
    printf("\n");


    g_free(clean);
    return array;
}
typedef struct {
    int nb_inputs;
    int **input;
    int *output;
    int nb_layer;
    int *nb_neurone;
    const gchar *name;
} ThreadData;
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

    for (int i = 0; i < td->nb_inputs; i++) free(inputs_d[i]);
    free(inputs_d);
    free(outputs_d);
    free(neurones_d);

    for (int i = 0; i < td->nb_inputs; i++) {
        if (td->input[i]) free(td->input[i]);
    }
    free(td->input);
    free(td->output);
    free(td->nb_neurone);

    cleanup_td:
        if (td->name) g_free(td->name);
        g_free(td);

        fprintf(stderr, ">>> thread_function: exit\n");
        fflush(stderr);
    return NULL;
}


gboolean close_loading_window(gpointer data) {
    GtkWidget *loading_window = GTK_WIDGET(data);
    gtk_widget_destroy(loading_window);
    return FALSE;
}


void on_submit(GtkWidget *widget, gpointer data) {
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
    gtk_window_set_default_size(GTK_WINDOW(loading_window), 200, 100);
    GtkWidget *label = gtk_label_new("Veuillez patienter...");
    gtk_container_add(GTK_CONTAINER(loading_window), label);
    gtk_widget_show_all(loading_window);


    ThreadData *td = g_malloc(sizeof(ThreadData));
    td->nb_inputs = nb_inputs;
    td->input = input;
    td->output = output;
    td->nb_layer = nb_layer;
    td->nb_neurone = nb_neurone;
    td->name = g_strdup(name); 

    g_timeout_add_seconds(7, close_loading_window, loading_window);


    GThread *thread = g_thread_new("worker", thread_function, td);
    
    g_thread_unref(thread);

}


void open_training_network_window(GtkWidget *widget, gpointer data) {
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

void open_testing_window(GtkWidget *widget, gpointer data) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Résolution d'image");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label = gtk_label_new("Upload your image to solve !");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

    gtk_widget_show_all(window);
}

     
// ------------------------------
// Nouvelle fenêtre : Entraînement
// ------------------------------
void open_training_window(GtkWidget *widget, gpointer data) {
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
