// File: hyprfiles.cpp

#include <gtkmm.h>
#include <filesystem>
#include <iostream>
#include <optional>

namespace fs = std::filesystem;

class HyprFilesWindow : public Gtk::Window {
public:
    HyprFilesWindow();

private:
    // UI Elements
    Gtk::Box main_box;
    Gtk::Box left_box;
    Gtk::Box right_box;
    Gtk::ListBox quick_access_list;
    Gtk::TreeView file_view;
    Gtk::ScrolledWindow scrolled_window;
    Gtk::Button paste_dead_space;
    Glib::RefPtr<Gtk::ListStore> file_list_store;

    // Clipboard
    std::optional<fs::path> clipboard_path;
    std::string clipboard_action; // "copy" or "cut"

    // File navigation
    std::string current_directory;
    bool show_hidden_files = false;

    // File list
    void load_directory(const std::string& path);
    void update_file_view();
    void on_quick_access_item_selected(Gtk::ListBoxRow* row);
    void on_file_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

    // Context Menu
    Gtk::Menu context_menu;
    void setup_context_menu();
    void on_menu_copy();
    void on_menu_cut();
    void on_menu_paste();
    void on_menu_undo();
    void on_menu_toggle_hidden();

    // Dead Space Click
    void on_dead_space_paste();

    // Columns for TreeView
    class FileColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        FileColumns() {
            add(col_icon);
            add(col_name);
            add(col_is_dir);
        }

        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> col_icon;
        Gtk::TreeModelColumn<std::string> col_name;
        Gtk::TreeModelColumn<bool> col_is_dir;
    };

    FileColumns file_columns;
};

HyprFilesWindow::HyprFilesWindow()
    : main_box(Gtk::ORIENTATION_HORIZONTAL, 5),
      left_box(Gtk::ORIENTATION_VERTICAL, 5),
      right_box(Gtk::ORIENTATION_VERTICAL, 5),
      paste_dead_space("Paste Here"),
      current_directory(fs::current_path()) {
    set_title("HyprFiles - File Manager");
    set_default_size(800, 600);

    // Set up main box
    add(main_box);
    main_box.pack_start(left_box, Gtk::PACK_SHRINK);
    main_box.pack_start(right_box, Gtk::PACK_EXPAND_WIDGET);

    // Quick Access Menu
    Gtk::Label* home_label = Gtk::manage(new Gtk::Label("Home"));
    Gtk::Label* pictures_label = Gtk::manage(new Gtk::Label("Pictures"));
    Gtk::Label* root_label = Gtk::manage(new Gtk::Label("/"));

    Gtk::ListBoxRow* home_row = Gtk::manage(new Gtk::ListBoxRow());
    home_row->add(*home_label);
    quick_access_list.append(*home_row);

    Gtk::ListBoxRow* pictures_row = Gtk::manage(new Gtk::ListBoxRow());
    pictures_row->add(*pictures_label);
    quick_access_list.append(*pictures_row);

    Gtk::ListBoxRow* root_row = Gtk::manage(new Gtk::ListBoxRow());
    root_row->add(*root_label);
    quick_access_list.append(*root_row);

    left_box.pack_start(quick_access_list, Gtk::PACK_EXPAND_WIDGET);
    quick_access_list.signal_row_selected().connect(
        sigc::mem_fun(*this, &HyprFilesWindow::on_quick_access_item_selected));

    // File View with Scroll
    file_list_store = Gtk::ListStore::create(file_columns);
    file_view.set_model(file_list_store);
    file_view.append_column("Icon", file_columns.col_icon);
    file_view.append_column("Name", file_columns.col_name);

    scrolled_window.add(file_view);
    right_box.pack_start(scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    file_view.signal_row_activated().connect(
        sigc::mem_fun(*this, &HyprFilesWindow::on_file_row_activated));

    // Dead space for paste
    paste_dead_space.signal_clicked().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_dead_space_paste));
    right_box.pack_start(paste_dead_space, Gtk::PACK_SHRINK);

    // Context Menu
    setup_context_menu();

    // Load the initial directory
    load_directory(current_directory);

    show_all_children();
}

void HyprFilesWindow::setup_context_menu() {
    auto menu_item_copy = Gtk::make_managed<Gtk::MenuItem>("Copy");
    menu_item_copy->signal_activate().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_menu_copy));
    context_menu.append(*menu_item_copy);

    auto menu_item_cut = Gtk::make_managed<Gtk::MenuItem>("Cut");
    menu_item_cut->signal_activate().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_menu_cut));
    context_menu.append(*menu_item_cut);

    auto menu_item_paste = Gtk::make_managed<Gtk::MenuItem>("Paste");
    menu_item_paste->signal_activate().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_menu_paste));
    context_menu.append(*menu_item_paste);

    auto menu_item_undo = Gtk::make_managed<Gtk::MenuItem>("Undo");
    menu_item_undo->signal_activate().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_menu_undo));
    context_menu.append(*menu_item_undo);

    auto menu_item_toggle_hidden = Gtk::make_managed<Gtk::MenuItem>("Show Hidden Files");
    menu_item_toggle_hidden->signal_activate().connect(sigc::mem_fun(*this, &HyprFilesWindow::on_menu_toggle_hidden));
    context_menu.append(*menu_item_toggle_hidden);

    context_menu.show_all();
}

void HyprFilesWindow::load_directory(const std::string& path) {
    current_directory = path;
    update_file_view();
}

void HyprFilesWindow::update_file_view() {
    file_list_store->clear();

    try {
        for (const auto& entry : fs::directory_iterator(current_directory)) {
            if (!show_hidden_files && entry.path().filename().string().starts_with("."))
                continue;

            auto row = *(file_list_store->append());
            row[file_columns.col_name] = entry.path().filename().string();

            if (entry.is_directory()) {
                row[file_columns.col_icon] = Gtk::IconTheme::get_default()->load_icon("folder", 24, Gtk::ICON_LOOKUP_USE_BUILTIN);
                row[file_columns.col_is_dir] = true;
            } else {
                row[file_columns.col_icon] = Gtk::IconTheme::get_default()->load_icon("text-x-generic", 24, Gtk::ICON_LOOKUP_USE_BUILTIN);
                row[file_columns.col_is_dir] = false;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading directory: " << e.what() << std::endl;
    }
}

void HyprFilesWindow::on_quick_access_item_selected(Gtk::ListBoxRow* row) {
    if (!row) return;

    Gtk::Label* label = dynamic_cast<Gtk::Label*>(row->get_child());
    if (!label) return;

    std::string label_text = label->get_text();
    if (label_text == "Home") {
        load_directory(fs::path(getenv("HOME")).string());
    } else if (label_text == "Pictures") {
        load_directory(fs::path(getenv("HOME")).append("Pictures").string());
    } else if (label_text == "/") {
        load_directory("/");
    }
}

void HyprFilesWindow::on_file_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    Gtk::TreeModel::iterator iter = file_list_store->get_iter(path);
    if (!iter) return;

    Gtk::TreeModel::Row row = *iter;
    std::string name = row[file_columns.col_name];
    bool is_dir = row[file_columns.col_is_dir];

    if (is_dir) {
        load_directory(current_directory + "/" + name);
    }
}

void HyprFilesWindow::on_menu_copy() {
    auto selected_row = file_view.get_selection()->get_selected();
    if (!selected_row) return;

    Gtk::TreeModel::Row row = *selected_row;
    clipboard_path = current_directory + "/" + row[file_columns.col_name];
    clipboard_action = "copy";
}

void HyprFilesWindow::on_menu_cut() {
    auto selected_row = file_view.get_selection()->get_selected();
    if (!selected_row) return;

    Gtk::TreeModel::Row row = *selected_row;
    clipboard_path = current_directory + "/" + row[file_columns.col_name];
    clipboard_action = "cut";
}

void HyprFilesWindow::on_menu_paste() {
    if (!clipboard_path || clipboard_action.empty()) return;

    try {
        fs::path destination = current_directory + "/" + clipboard_path->filename().string();
        if (clipboard_action == "copy") {
            fs::copy(*clipboard_path, destination, fs::copy_options::recursive);
        } else if (clipboard_action == "cut") {
            fs::rename(*clipboard_path, destination);
            clipboard_path.reset();
            clipboard_action.clear();
        }
        update_file_view();
    } catch (const std::exception& e) {
        std::cerr << "Paste operation failed: " << e.what() << std::endl;
    }
}

void HyprFilesWindow::on_menu_undo() {
    clipboard_action.clear();
    clipboard_path.reset();
}

void HyprFilesWindow::on_menu_toggle_hidden() {
    show_hidden_files = !show_hidden_files;
    update_file_view();
}

void HyprFilesWindow::on_dead_space_paste() {
    on_menu_paste();
}

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.hyprfiles.filemanager");
    HyprFilesWindow window;
    return app->run(window);
}

