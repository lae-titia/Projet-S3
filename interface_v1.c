#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "neurone_systemImage.h"
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



int main (int argc, char *argv[])
{
	//interface widgets 
	GtkWidget *window;
	GtkWidget *buttonUpload;
	GtkWidget *buttonTraining;
	GtkWidget *label;
	GtkWidget *box;
	GtkWidget *space;
	GtkCssProvider *css_;
	GtkWidget *hbox;	
	gtk_init(&argc, &argv);
		
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	main_window = window;
	gtk_window_set_title (GTK_WINDOW (window), "SolvLad");
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
		
	//Main window
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
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}


void _clicked_button_upload(GtkWidget *widget, gpointer user_data)
{
    	GtkWidget *vbox = GTK_WIDGET(user_data); 
    	GtkWidget *window = gtk_widget_get_toplevel(widget);
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(window,"key-press-event", G_CALLBACK(_on_key_press),NULL);
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
		
			ImageData *img_data = g_new0(ImageData, 1);
		img_data->image = image;
		img_data->pixbuf = gdk_pixbuf_copy(pixbuf);
		img_data->angle = 0.0;

		g_object_set_data(G_OBJECT(window), "image_data", img_data);
	        gtk_widget_hide(widget); 
                gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
                gtk_widget_show(image);
		g_object_unref(pixbuf);
                g_object_unref(scaled);
            } 
	    else 
	    {
		    g_print("Loading error: %s\n", error->message);
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
    gtk_window_set_title(GTK_WINDOW(window), "Image resolution");
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


gboolean training_finished_callback(gpointer data) {
    GtkWidget *loading_label = GTK_WIDGET(data);
    gtk_label_set_text(GTK_LABEL(loading_label), "Training terminé !");
    

    return FALSE; 
}


void _clicked_upload(GtkWidget *widget, gpointer data) {
    GtkWidget *parent = GTK_WIDGET(data); // le vbox ou la fenêtre
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select an image",
        GTK_WINDOW(gtk_widget_get_toplevel(parent)),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(chooser);
        g_print("Fichier sélectionné : %s\n", filename);

        // stocke le chemin dans le bouton pour pouvoir le récupérer plus tard
        g_object_set_data(G_OBJECT(widget), "uploaded_file", filename);
    }

    gtk_widget_destroy(dialog);
}


void move_file_to_letter_folder(const char *filepath, char letter) {
   
    if (letter >= 'a' && letter <= 'z') letter -= 32;

    const char *filename = strrchr(filepath, '/'); 
    if (!filename) filename = filepath;
    else filename++;

    char newpath[512];
    snprintf(newpath, sizeof(newpath), "./lettres/%c/%s", letter, filename);
    printf("%s = %s ??\n", newpath, filename);

    if (access(filepath, F_OK) != 0) perror("Source inexistante");

    if (rename(filepath, newpath) == 0) {
        g_print("Fichier déplacé vers : %s\n", newpath);
    } else {
        perror("Erreur déplacement fichier");
    }
}

typedef struct {
    char *filename;
    char letter;
} ThreadPrincipalData;

gpointer thread_principal(gpointer data) {
    ThreadPrincipalData *td = (ThreadPrincipalData *)data;

    printf("Thread démarré\n");
    
    principal();

    g_idle_add((GSourceFunc)training_finished_callback, loading_label);

    g_free(td->filename);
    g_free(td);

    printf("Thread terminé\n");
    return NULL;
}

void on_submit_neurone(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    GtkWidget *upload_button = entries[0]; 
    GtkWidget *entry_input = entries[1];    

    char *filename = g_object_get_data(G_OBJECT(upload_button), "uploaded_file");
    const char *letter_text = gtk_entry_get_text(GTK_ENTRY(entry_input));

    if (!filename) {
        g_print("Aucun fichier sélectionné !\n");
        return;
    }
    if (!letter_text || strlen(letter_text) != 1) {
        g_print("Veuillez entrer une seule lettre !\n");
        return;
    }

    char letter = letter_text[0];
    move_file_to_letter_folder(filename, letter);
    g_print("Avant création thread\n");
    ThreadPrincipalData *td = g_malloc(sizeof(ThreadPrincipalData));
    td->filename = g_strdup(filename);
    td->letter = letter;

    GThread *thread = g_thread_new("worker", thread_principal, td);
    g_thread_unref(thread);
    

}

void open_training_network_window(GtkWidget *widget, gpointer data) {
    
    (void)widget;
    (void)data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Image to learn");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label = gtk_label_new("Upload your image to learn!");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

    GtkWidget *buttonImage =  gtk_button_new_with_label("Upload");
    gtk_box_pack_start(GTK_BOX(vbox), buttonImage, TRUE, TRUE, 0);
    gtk_widget_set_size_request(buttonImage, 100, 50);
    g_signal_connect(buttonImage, "clicked", G_CALLBACK(_clicked_upload), vbox);
    GtkWidget *label_input = gtk_label_new("Enter the letter associated");
    GtkWidget *entry_input = gtk_entry_new();
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_attach(GTK_GRID(grid), label_input, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_input, 1, 0, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
    GtkWidget *button = gtk_button_new_with_label("Start learning");
    gtk_widget_set_size_request(button, 200, 50);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 10);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    GtkWidget **entries = g_new(GtkWidget*, 2);
    entries[0] = buttonImage;
    entries[1] = entry_input;
    g_signal_connect(button, "clicked", G_CALLBACK(on_submit_neurone), entries);
    

    gtk_widget_show_all(window);
    
}



void on_submit_prediction(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    GtkWidget *upload_button = entries[0]; 

    char *filename = g_object_get_data(G_OBJECT(upload_button), "uploaded_file");
    if (!filename) {
        g_print("Aucun fichier sélectionné !\n");
        return;
    }

    char *lettre = malloc(sizeof(char));
    prediction(filename, lettre);
    printf("La lettre est : %s", lettre);
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Prediction: %s", lettre);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Résultat");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *label = gtk_label_new(buffer);
    gtk_container_add(GTK_CONTAINER(window), label);
    gtk_widget_show_all(window);


}


void open_testing_window(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Letter to predict");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label = gtk_label_new("Upload your image to predict!");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

    GtkWidget *buttonImage =  gtk_button_new_with_label("Upload");
    gtk_box_pack_start(GTK_BOX(vbox), buttonImage, TRUE, TRUE, 0);
    gtk_widget_set_size_request(buttonImage, 100, 50);
    g_signal_connect(buttonImage, "clicked", G_CALLBACK(_clicked_upload), vbox);
    GtkWidget *button = gtk_button_new_with_label("Predict");
    gtk_widget_set_size_request(button, 200, 50);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 10);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    GtkWidget **entries = g_new(GtkWidget*, 2);
    entries[0] = buttonImage;
    g_signal_connect(button, "clicked", G_CALLBACK(on_submit_prediction), entries);
    

    gtk_widget_show_all(window);
}




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
	(void)data_useless;
	ImageData *img_data = g_object_get_data(G_OBJECT(window), "image_data");
	if (!img_data || !img_data->pixbuf) 
	{
        	g_print("No image to load\n");
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
   	    	g_print("Image rotate at %.1f degrees\n", img_data->angle);
  	} 
	else 
	{
        	g_print("Error when rotating image!\n");
   	}
	
	return TRUE;
}



