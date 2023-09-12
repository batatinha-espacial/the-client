#include <gtk/gtk.h>
#include <vector>
#include <memory>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio.hpp>
#include <iomanip>
#include <sstream>

enum TCP_MODE {
    TCP_HEXADECIMAL
};

std::string byteToHex(uint8_t byte) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte);
    std::string result = stream.str();
    return result;
}

std::vector<uint8_t> streambufToBytes(boost::asio::streambuf& streambuf) {
    std::vector<uint8_t> bytes;

    // Get the size of the streambuf
    std::size_t size = streambuf.size();

    // Reserve space in the byte vector to hold the data
    bytes.reserve(size);

    // Create an input stream from the streambuf
    std::istream is(&streambuf);

    // Read the data from the streambuf into the byte vector
    while (size > 0) {
        uint8_t byte;
        is.read(reinterpret_cast<char*>(&byte), 1);
        bytes.push_back(byte);
        --size;
    }

    return bytes;
}

struct ClientTcpWindow {
    GtkWidget* win;
    TCP_MODE mode;
    GtkWidget *main_box, *top_bar, *content, *bottom_bar;
    GtkWidget* text_view;
    GtkTextBuffer* text_buffer;
    GtkWidget *addr, *addr_input;
    GtkEntryBuffer* addr_buffer;
    GtkWidget *port, *port_input;
    GtkEntryBuffer* port_buffer;
    GtkWidget* connect_btn;
    bool content_initialized;
    std::shared_ptr<boost::asio::ip::tcp::socket> conn;
    bool connected = false;
    std::future<void> receive;
};

struct ClientMainWindow {
    GtkWidget *window, *content, *grid, *tcp_hexadecimal;
};

struct ClientData {
    GtkApplication* app;
    ClientMainWindow main_window;
    std::vector<std::shared_ptr<ClientTcpWindow>> tcp_wins;
};

void gtk_text_buffer_insert_at_end(GtkTextBuffer* buffer, const char *text) {
    GtkTextIter* end;
    gtk_text_buffer_get_end_iter(buffer, end);
    gtk_text_buffer_insert(buffer, end, text, -1);
}

void client_insert_or_set(GtkWidget* text_view, GtkTextBuffer* text_buffer, const char* toinsert, ClientTcpWindow* tcp) {
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), true);
    if (!tcp->content_initialized) {
        gtk_text_buffer_set_text(text_buffer, toinsert, -1);
        tcp->content_initialized = false;
    } else {
        gtk_text_buffer_insert_at_end(text_buffer, toinsert);
        gtk_text_buffer_insert_at_end(text_buffer, toinsert);
    }
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
}

// Function used to make the layout on some windows (Pattern 1)
// Pattern 1 has a topbar, a bottombar, and the main content between those bars
std::vector<GtkWidget*> client_mklayout1() {
    // Define the box that will contain the layout
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    // Define the topbar
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(main_box), top_bar);
    // Define the main content as a scrollable widget
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_box_append(GTK_BOX(main_box), scrolled_window);
    // Define the bottombar
    GtkWidget* bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(main_box), bottom_bar);
    // Make a vector that will contain those widgets in the order:
    // Main box, topbar, main content, bottombar
    // And return the vector
    std::vector<GtkWidget*> result;
    result.push_back(main_box);
    result.push_back(top_bar);
    result.push_back(scrolled_window);
    result.push_back(bottom_bar);
    return result;
}

void client_tcp_receive(gpointer win) {
    ClientData* data = (ClientData*)g_object_get_data(G_OBJECT(win), "data");
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "index"));
    auto text_buffer = data->tcp_wins.at(index)->text_buffer;
    auto text_view = data->tcp_wins.at(index)->text_view;
    auto socket = data->tcp_wins.at(index)->conn;
    auto mode = data->tcp_wins.at(index)->mode;
    try {
        while (true) {
            boost::system::error_code error;
            boost::asio::streambuf buf;
            boost::asio::read(*(socket.get()), buf.prepare(1), boost::asio::transfer_exactly(1), error);
            buf.commit(1);
            if (error != boost::asio::error::eof && error) {
                throw boost::system::system_error(error);
            }
            
            std::vector<uint8_t> byteArray = streambufToBytes(buf);
            if (byteArray.size() != 0) {
                std::string receivedData = "< ";
                if (mode == TCP_HEXADECIMAL) {
                    for (auto byte : byteArray) {
                        receivedData += byteToHex(byte);
                    }
                }
                client_insert_or_set(text_view, text_buffer, receivedData.c_str(), data->tcp_wins.at(index).get());
            }
            if (error == boost::asio::error::eof) {
                client_insert_or_set(text_view, text_buffer, "Clonnection closed by server.", data->tcp_wins.at(index).get());
                break;
            }
        }
    } catch (const std::exception& e) {
        std::string toinsert = "Error: ";
        toinsert += e.what();
        client_insert_or_set(text_view, text_buffer, toinsert.c_str(), data->tcp_wins.at(index).get());
        return;
    }
}

void client_tcp_connect(GtkWidget* self, gpointer win) {
    ClientData* data = (ClientData*)g_object_get_data(G_OBJECT(win), "data");
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "index"));
    auto addr_buffer = data->tcp_wins.at(index)->addr_buffer;
    auto port_buffer = data->tcp_wins.at(index)->port_buffer;
    auto text_buffer = data->tcp_wins.at(index)->text_buffer;
    auto text_view = data->tcp_wins.at(index)->text_view;
    auto addr_text = (std::string)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(addr_buffer));
    auto port_text = (std::string)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(port_buffer));
    int maximum_port = 65535;
    boost::algorithm::trim(addr_text);
    boost::algorithm::trim(port_text);
    int port_num;
    if (data->tcp_wins.at(index)->connected) {
        client_insert_or_set(text_view, text_buffer, "Error: cannot connect to 2 servers at the same time.", data->tcp_wins.at(index).get());
        return;
    }
    try {
        port_num = std::stoi(port_text);
    } catch (const std::exception &e) {
        client_insert_or_set(text_view, text_buffer, "Error: invalid port number.", data->tcp_wins.at(index).get());
        return;
    }
    if (!(port_num <= maximum_port)) {
        client_insert_or_set(text_view, text_buffer, "Error: maximum port number is 65535.", data->tcp_wins.at(index).get());
        return;
    }
    if (port_num == 0) {
        client_insert_or_set(text_view, text_buffer, "Error: port number can't be zero.", data->tcp_wins.at(index).get());
        return;
    }
    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::io_service io_service;
    try {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(addr_text), port_num);
    } catch (const boost::system::system_error &e) {
        try {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(addr_text, std::to_string(port_num));
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            endpoint = *endpoint_iterator;
        } catch (const boost::system::system_error &e) {
            std::string toinsert = "Error: not a valid IPv4 address or invalid/non-existing domain name: ";
            toinsert += e.what();
            client_insert_or_set(text_view, text_buffer, toinsert.c_str(), data->tcp_wins.at(index).get());
        }
    }
    data->tcp_wins.at(index)->conn = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
    try {
        data->tcp_wins.at(index)->conn->connect(endpoint);
    } catch (const boost::system::system_error &e) {
        std::string toinsert = "Error while connecting to specified address: ";
        toinsert += e.what();
        client_insert_or_set(text_view, text_buffer, toinsert.c_str(), data->tcp_wins.at(index).get());
        return;
    }
    std::string toinsert = "Successfully connected to address \"";
    toinsert += addr_text;
    toinsert += "\" on port ";
    toinsert += std::to_string(port_num);
    toinsert += ".";
    client_insert_or_set(text_view, text_buffer, toinsert.c_str(), data->tcp_wins.at(index).get());
    data->tcp_wins.at(index)->connected = true;
    data->tcp_wins.at(index)->receive = std::async(std::launch::async, [&win]() {
        client_tcp_receive(win);
    });
}

// Start TCP Client
void client_tcp_start(TCP_MODE mode, gpointer data_gptr) {
    ClientData* data = (ClientData*)data_gptr;
    data->tcp_wins.push_back(std::make_shared<ClientTcpWindow>());
    int index = data->tcp_wins.size() - 1;
    // Define the Window
    GtkWidget *tcp_win = gtk_window_new();
    g_object_set_data(G_OBJECT(tcp_win), "data", data);
    g_object_set_data(G_OBJECT(tcp_win), "index", GINT_TO_POINTER(index));
    data->tcp_wins.at(index)->win = tcp_win;
    data->tcp_wins.at(index)->mode = mode;
    // Setup the window based on the mode
    if (mode == TCP_HEXADECIMAL) gtk_window_set_title(GTK_WINDOW(tcp_win), "The Client - TCP (Hexadecimal)");
    gtk_window_set_default_size(GTK_WINDOW(tcp_win), 750, 750);
    // Make the layout of the window
    auto layout = client_mklayout1();
    auto main_box = layout.at(0);
    data->tcp_wins.at(index)->main_box = main_box;
    auto top_bar = layout.at(1);
    data->tcp_wins.at(index)->top_bar = top_bar;
    auto content = layout.at(2);
    data->tcp_wins.at(index)->content = content;
    auto bottom_bar = layout.at(3);
    data->tcp_wins.at(index)->bottom_bar = bottom_bar;
    // Setup the topbar
    // Set up address label
    GtkWidget* addr = gtk_label_new("Address:");
    data->tcp_wins.at(index)->addr = addr;
    gtk_box_append(GTK_BOX(top_bar), addr);
    // Set up address input
    GtkWidget* addr_input = gtk_entry_new();
    data->tcp_wins.at(index)->addr_input = addr_input;
    gtk_box_append(GTK_BOX(top_bar), addr_input);
    // Set up address buffer
    GtkEntryBuffer* addr_buffer = gtk_entry_get_buffer(GTK_ENTRY(addr_input));
    data->tcp_wins.at(index)->addr_buffer = addr_buffer;
    // Set up port label
    GtkWidget* port = gtk_label_new("Port:");
    data->tcp_wins.at(index)->port = port;
    gtk_box_append(GTK_BOX(top_bar), port);
    // Set up port input
    GtkWidget* port_input = gtk_entry_new();
    data->tcp_wins.at(index)->port_input = port_input;
    gtk_box_append(GTK_BOX(top_bar), port_input);
    // Set up port buffer
    GtkEntryBuffer* port_buffer = gtk_entry_get_buffer(GTK_ENTRY(port_input));
    data->tcp_wins.at(index)->port_buffer = port_buffer;
    // Set up connect button
    GtkWidget* connect_btn = gtk_button_new_with_label("Connect");
    data->tcp_wins.at(index)->connect_btn = connect_btn;
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(client_tcp_connect), tcp_win);
    gtk_box_append(GTK_BOX(top_bar), connect_btn);
    // Setup the content
    GtkWidget* text_view = gtk_text_view_new();
    data->tcp_wins.at(index)->text_view = text_view;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    data->tcp_wins.at(index)->text_buffer = text_buffer;
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), false);
    gtk_text_buffer_set_text(text_buffer, "Connect to a server to see some content.", -1);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
    data->tcp_wins.at(index)->content_initialized = false;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(content), text_view);
    // Show the window
    gtk_window_set_child(GTK_WINDOW(tcp_win), main_box);
    gtk_window_present(GTK_WINDOW(tcp_win));
}

// Function used when the "TCP (Hexadecimal)" button on the main window is clicked
void client_tcp_hexadecimal_button(GtkWidget *self, gpointer data_gptr) {
    // Start TCP Client
    client_tcp_start(TCP_HEXADECIMAL, data_gptr);
}

// Function used when main window is activated
void client_activate(GtkApplication *app, gpointer data_gptr) {
    ClientData* data = (ClientData*)data_gptr;
    // GtkWidget Definitions
    GtkWidget *window, *content, *grid, *tcp_hexadecimal;
    // Main Window Setup
    window = gtk_application_window_new(app);
    data->main_window.window = window;
    gtk_window_set_title(GTK_WINDOW(window), "The Client - Menu");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
    // "TCP (Hexadecimal)" button Setup
    tcp_hexadecimal = gtk_button_new_with_label("TCP (Hexadecimal)");
    data->main_window.tcp_hexadecimal = tcp_hexadecimal;
    g_signal_connect(tcp_hexadecimal, "clicked", G_CALLBACK(client_tcp_hexadecimal_button), data);
    // Grid Setup
    grid = gtk_grid_new();
    data->main_window.grid = grid;
    gtk_grid_attach(GTK_GRID(grid), tcp_hexadecimal, 1, 1, 1, 1);
    // Make the Grid Scrollable
    content = gtk_scrolled_window_new();
    data->main_window.content = content;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(content), grid);
    gtk_window_set_child(GTK_WINDOW(window), content);
    // Show Main Window
    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
}

// Run "the-client" app
int client_run_app(int argc, char** argv) {
    // Define the shared data
    ClientData data;
    // Define the app
    GtkApplication *app = gtk_application_new("org.the.client", G_APPLICATION_DEFAULT_FLAGS);
    data.app = app;
    // Run the app and return the status. This part should not be touched
    int status;
    g_signal_connect(app, "activate", G_CALLBACK(client_activate), &data);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}