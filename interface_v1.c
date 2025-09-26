#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
	//Les widgets de l'interface 
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *box;
	GtkWidget *space;
	GtkCssProvider *css_;

	//Initialise GTK
	gtk_init(&argc, &argv);

	//Inisialiser les widgets
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "SolvLad");
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
	
	label = gtk_label_new("Welcome to SolvLab");
	space = gtk_label_new(NULL);
	button = gtk_button_new_with_label("Upload");

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    	gtk_container_add(GTK_CONTAINER(window), box);
	
	gtk_box_pack_start (GTK_BOX(box), label, TRUE, TRUE, 0);
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_name(label, "title");
	
    	gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(box), space, TRUE, TRUE, 0);


	//Utilisation de CSS pour styliser l'interface
	css_ = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css_,
	"window{\n"
	" background-color : #72AD6D;\n}"
	"#title{\n"
	"    font-family: 'Fira Code', monospace;\n"
        "    font-size: 20px;\n"
        "    font-weight: bold;\n"
        "    color: white;\n}", -1, NULL);
	
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
