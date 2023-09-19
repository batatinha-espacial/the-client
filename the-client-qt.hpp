#include <qt6/QtWidgets/QApplication>
#include <qt6/QtGui/QWindow>

struct Client {
    // The application
    QApplication* app;
    // A data type to store the menu window and all things related to it
    struct menu_win_t {
        // The menu window
        QWindow* win;
        void init() {
            this->win = new QWindow;
        }
        void default_title() {
            this->win->setTitle("The Client - Menu");
        }
        // The class initializer (should not be touched)
        menu_win_t() {}
        // The class destructor
        ~menu_win_t() {
            delete this->win;
        }
        void show() {
            this->win->show();
        }
        void default_size() {
            this->win->setWidth(500);
            this->win->setHeight(500);
        }
        void setup() {
            this->init();
            this->default_title();
            this->default_size();
            this->show();
        }
    };
    // The data about the menu window
    menu_win_t* menu;
    // Initialize the application
    void init_app(int argc, char **argv) {
        this->app = new QApplication(argc, argv);
    }
    // Initialize the `menu` struct
    void init_menu() {
        this->menu = new menu_win_t;
    }
    // Execute the app
    int exec() {
        return this->app->exec();
    }
    // Call all initializers
    void init(int argc, char** argv) {
        this->init_app(argc, argv);
        this->init_menu();
    }
    // Run the app
    int run(int argc, char **argv) {
        // Initialize structures
        this->init(argc, argv);
        // Setup the `menu`
        this->menu->setup();
        // Execute the app and return the exit code (should not be touched)
        return this->exec();
    }
    // The class initializer (should not be touched)
    Client() {}
    // The class destructor
    ~Client() {
        delete this->menu;
        delete this->app;
    }
};