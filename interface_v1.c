#include <gtk/gtk.h>

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
void open_resolution_window(GtkWidget *widget, gpointer data) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Résolution d'image");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *label = gtk_label_new("Ici tu vas charger une image et la résoudre !");
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);
}

// ------------------------------
// Nouvelle fenêtre : Entraînement
// ------------------------------
void open_training_window(GtkWidget *widget, gpointer data) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Session d'entraînement");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *label = gtk_label_new("Ici tu vas entraîner ton réseau de neurones !");
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);
}
