// Widget demo for tui++: the Swing-style toggleable widgets, the switch, the
// combo box, the context menus, the dialogs and the translations on one frame.
//
//     WidgetDemo            text screen  (cell-based escape sequences)
//     WidgetDemo text       same as the default
//     WidgetDemo sixel      sixel screen (pixel-based graphics)
//
// What to try:
//   - click the check boxes, radio buttons and the toggle button; radio
//     buttons and the grouped check boxes are exclusive per group,
//   - the switch: a toggle button drawn as a horizontal track with rounded
//     ends and a round thumb. It drives the same "guides" state as the check
//     boxes (and the dialog's), so all three stay in step,
//   - the shadowed "Reset" button casts a drop shadow: the room it takes is
//     part of the button's box, so the layout places the widgets after it
//     accordingly (a program sets a button shadow where it wants one),
//   - the View menu holds a check box menu item and a Density submenu whose
//     radio items are grouped,
//   - the two combo boxes: arrow keys open the dropdown, Up/Down/Home/End
//     move the highlight, Enter picks, Escape cancels. The "City" combo is
//     editable: typing looks the item up ("lookup editing"), Enter commits
//     the match or keeps custom text; the "Font size" combo is read-only and
//     picks the item you type letters for.
//   - right-click the comment field (or press Shift+F10 in it) for its
//     standard text menu -- Undo, Redo, Cut, Copy, Paste, Delete, Select All
//     -- which every text component carries for free and every row of it
//     enables from the field's own state,
//   - right-click the panel or a heading for the demo's own context menu (a
//     check box row, plain rows, separators, a submenu of radio rows, and the
//     rows that open the dialogs). The headings inherit the panel's menu
//     (Swing's setInheritsPopupMenu); the buttons keep their own gestures,
//   - the Dialogs menu: "Modal Dialog..." opens a dialog that holds the
//     frame's whole input until it is closed -- its set_visible(true) does not
//     return while it is up, so the status line is updated by the code after
//     the call -- with OK (or Enter: it is the dialog's default button),
//     Cancel and Escape. "Modeless Dialog..." stays up while the frame keeps
//     working, and the two "Show guides" check boxes drive the same state,
//   - the Language menu: English, Deutsch, Français, Русский and 中文 switch the
//     locale of every live widget. A widget's own text is the key of the lookup
//     (see Messages), and the look-and-feel does the lookup when it measures and
//     paints, so the bundles installed at the top of main turn "Bold" into
//     "Fett" and "File" into "Datei" on the spot -- texts and layouts both,
//     with no translation API on the components. The language names themselves
//     opt out through a client property, and the demo's own headings show how a
//     program translates its own strings (WidgetDemoTextLine::paint).
//
// The bottom status line summarizes every control after each change.

#include <tui++/BorderLayout.h>
#include <tui++/BoxLayout.h>
#include <tui++/Button.h>
#include <tui++/ButtonGroup.h>
#include <tui++/CheckBox.h>
#include <tui++/CheckBoxMenuItem.h>
#include <tui++/ComboBox.h>
#include <tui++/Dialog.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Messages.h>
#include <tui++/Panel.h>
#include <tui++/PopupMenu.h>
#include <tui++/RadioButton.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/RootPane.h>
#include <tui++/Shadow.h>
#include <tui++/Switch.h>
#include <tui++/TextField.h>
#include <tui++/ToggleButton.h>

#include <tui++/border/EmptyBorder.h>

#include <tui++/terminal/Terminal.h>

#include <memory>
#include <string>
#include <string_view>

using namespace tui;

namespace tui {

// A one-line panel that paints its text (a heading or the status line).
//
// The text belongs to the program, and the class shows how a program
// translates its own strings: paint asks Messages for the current locale (the
// toolkit's own labels are looked up the same way, inside their look-and-feel)
// and the language menu's repaint refreshes every line.
class WidgetDemoTextLine: public Panel {
  std::string text;

public:
  void set_text(std::string const &text) {
    if (this->text != text) {
      this->text = text;
      repaint();
    }
  }

  void paint(Graphics &g) override {
    if (auto translated = Messages::get(this->text); not translated.empty()) {
      g.draw_string(translated, 0, 0);
    }
  }

protected:
  WidgetDemoTextLine(std::string const &text = "") :
      text(text) {
    set_preferred_size(Dimension { 0, 1 });
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

// A horizontal row of the demo's control panel.
class WidgetDemoRow: public Panel {
protected:
  WidgetDemoRow() = default;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

std::string widget_on_off(bool value) {
  return value ? "on" : "off";
}

std::string widget_selected_text(std::shared_ptr<AbstractButton> const &button) {
  // The text the widget shows in the current locale, so the status line names
  // the state the way the buttons do.
  return button->is_selected() ? Messages::get(button->get_text()) : std::string { };
}

}

using namespace tui;

int main(int argc, char *argv[]) {
  auto type = argc > 1 ? std::string(argv[1]) : std::string { "text" };
  terminal.set_type(type);

  // ---- the translation bundles ----
  //
  // A widget's text is the key of the lookup (see Messages), so every entry
  // names the original phrase: the buttons and menu rows below read "Bold",
  // "File" and so on, and the Language menu shows them in the locale it picks.
  // The demo installs its bundles with put(); a translator's properties file
  // would be loaded with Messages::load instead. Only the toolkit's own texts
  // are translated -- the headings and the status line are the program's own
  // strings, looked up by hand in WidgetDemoTextLine.
  struct Translation {
    std::string_view key;
    std::string_view de;
    std::string_view fr;
    std::string_view ru;
    std::string_view zh; // Simplified Chinese
  };

  static constexpr Translation translations[] = {
    // The menu bar and its rows.
    { "File", "Datei", "Fichier", "Файл", "文件" },
    { "View", "Ansicht", "Affichage", "Вид", "视图" },
    { "Dialogs", "Dialoge", "Dialogues", "Диалоги", "对话框" },
    { "Language", "Sprache", "Langue", "Язык", "语言" },
    { "Quit", "Beenden", "Quitter", "Выход", "退出" },
    { "Word wrap", "Zeilenumbruch", "Retour à la ligne", "Перенос по словам", "自动换行" },
    { "Density", "Dichte", "Densité", "Плотность", "密度" },
    { "Compact", "Kompakt", "Compact", "Компактный", "紧凑" },
    { "Comfortable", "Komfortabel", "Confortable", "Удобный", "舒适" },
    { "Roomier", "Geräumig", "Spacieux", "Просторный", "宽松" },
    { "Modal Dialog...", "Modaler Dialog...", "Dialogue modal...", "Модальный диалог...", "模态对话框..." },
    { "Modeless Dialog...", "Nichtmodaler Dialog...", "Dialogue non modal...", "Немодальный диалог...", "非模态对话框..." },

    // The controls on the panel.
    { "Bold", "Fett", "Gras", "Полужирный", "粗体" },
    { "Italic", "Kursiv", "Italique", "Курсив", "斜体" },
    { "Underline", "Unterstrichen", "Souligné", "Подчёркнутый", "下划线" },
    { "Align Left", "Linksbündig", "Aligné à gauche", "По левому краю", "左对齐" },
    { "Align Center", "Zentriert", "Centré", "По центру", "居中" },
    { "Align Right", "Rechtsbündig", "Aligné à droite", "По правому краю", "右对齐" },
    { "Snap to grid", "Am Raster ausrichten", "Aimantation à la grille", "Привязать к сетке", "对齐网格" },
    { "Show guides", "Hilfslinien anzeigen", "Afficher les repères", "Показывать направляющие", "显示参考线" },
    { "Reset", "Zurücksetzen", "Réinitialiser", "Сбросить", "重置" },

    // The standard text menu every text component carries (see TextPopupMenu):
    // its rows are ordinary menu items, so a program may translate them too.
    { "Undo", "Rückgängig", "Annuler", "Отменить", "撤销" },
    { "Redo", "Wiederholen", "Rétablir", "Повторить", "重做" },
    { "Cut", "Ausschneiden", "Couper", "Вырезать", "剪切" },
    { "Copy", "Kopieren", "Copier", "Копировать", "复制" },
    { "Paste", "Einfügen", "Coller", "Вставить", "粘贴" },
    { "Delete", "Löschen", "Supprimer", "Удалить", "删除" },
    { "Select All", "Alles auswählen", "Tout sélectionner", "Выделить всё", "全选" },

    // The dialog contents (a window's own title is the program's string, not a
    // toolkit-drawn label, and stays as it is).
    { "The comment line:", "Die Kommentarzeile:", "La ligne de commentaire :", "Строка комментария:", "注释行：" },
    { "OK", "OK", "OK", "ОК", "确定" },
    { "Cancel", "Abbrechen", "Annuler", "Отмена", "取消" },
    { "The dialog and the frame stay live:",
      "Der Dialog und das Hauptfenster bleiben aktiv:",
      "Le dialogue et la fenêtre restent actifs :",
      "Диалог и главное окно остаются активными:",
      "对话框与主窗口保持活动：" },
    { "Close", "Schließen", "Fermer", "Закрыть", "关闭" },

    // The rows of the demo's context menu ("Alignment" and its radio rows share
    // the translations of the frame's own controls above).
    { "Select All in Comment", "Alles im Kommentar auswählen", "Tout sélectionner dans le commentaire", "Выделить всё в комментарии", "全选注释" },
    { "Clear Comment", "Kommentar löschen", "Effacer le commentaire", "Очистить комментарий", "清除注释" },
    { "Alignment", "Ausrichtung", "Alignement", "Выравнивание", "对齐" },

    // The headings, the program's own strings (WidgetDemoTextLine looks them up).
    { "Text style (toggle button, check boxes):",
      "Textstil (Umschalter, Kontrollkästchen):",
      "Style du texte (bouton bascule, cases à cocher) :",
      "Стиль текста (кнопка-переключатель, флажки):",
      "文本样式（切换按钮、复选框）：" },
    { "Alignment (radio buttons in one group):",
      "Ausrichtung (Optionsfelder einer Gruppe):",
      "Alignement (boutons radio d'un groupe) :",
      "Выравнивание (переключатели в одной группе):",
      "对齐（同组单选按钮）：" },
    { "Snapping (grouped check boxes):",
      "Einrasten (gruppierte Kontrollkästchen):",
      "Aimantation (cases à cocher groupées) :",
      "Привязка (сгруппированные флажки):",
      "吸附（分组复选框）：" },
    { "Combo boxes (editable lookup / dropdown):",
      "Kombinationsfelder (editierbare Suche / Dropdown):",
      "Listes déroulantes (recherche éditable / menu) :",
      "Комбинированные списки (поиск с вводом / раскрытие):",
      "组合框（可编辑查找 / 下拉）：" },
    { "Text field (caret, selection, clipboard, context menu):",
      "Textfeld (Cursor, Auswahl, Zwischenablage, Kontextmenü):",
      "Champ de texte (curseur, sélection, presse-papiers, menu contextuel) :",
      "Текстовое поле (курсор, выделение, буфер обмена, контекстное меню):",
      "文本框（光标、选择、剪贴板、上下文菜单）：" },
    { "Switch (rounded track) and a shadowed button:",
      "Schalter (runde Spur) und ein schattierter Knopf:",
      "Interrupteur (piste arrondie) et un bouton ombré :",
      "Переключатель (округлая дорожка) и кнопка с тенью:",
      "开关（圆角轨道）与带阴影的按钮：" },
  };

  for (auto &&translation : translations) {
    Messages::put("de", translation.key, translation.de);
    Messages::put("fr", translation.key, translation.fr);
    Messages::put("ru", translation.key, translation.ru);
    Messages::put("zh", translation.key, translation.zh);
  }

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });

  // ---- the controls whose state the status line summarizes ----
  auto bold = make_component<ToggleButton>("Bold");
  auto italic = make_component<CheckBox>("Italic");
  auto underline = make_component<CheckBox>("Underline");

  auto alignment_group = std::make_shared<ButtonGroup>();
  auto align_left = make_component<RadioButton>("Align Left");
  auto align_center = make_component<RadioButton>("Align Center");
  auto align_right = make_component<RadioButton>("Align Right");
  alignment_group->add(align_left);
  alignment_group->add(align_center);
  alignment_group->add(align_right);

  // A group of check boxes: the group only arbitrates NEW selections, so
  // clicking the selected member clears it again (Swing semantics).
  auto toggles_group = std::make_shared<ButtonGroup>();
  auto snap = make_component<CheckBox>("Snap to grid");
  auto guides = make_component<CheckBox>("Show guides");
  toggles_group->add(snap);
  toggles_group->add(guides);

  // A switch: a toggle button drawn as a horizontal track with a round thumb.
  // It drives the same "guides" state as the check box (and the modeless
  // dialog's), so the three stay in sync -- a toolbar switch and a menu check
  // that follow one command.
  auto switch_guides = make_component<Switch>("Show guides");
  switch_guides->add_listener([guides](ActionEvent &e) {
    guides->set_selected(std::static_pointer_cast<AbstractButton>(e.source)->is_selected());
  });
  guides->add_listener([guides, switch_guides](ActionEvent &) {
    switch_guides->set_selected(guides->is_selected());
  });

  auto city = make_component<ComboBox>(std::vector<std::string> { "Paris", "London", "Rome", "Berlin", "Madrid", "Amsterdam", "Prague", "Vienna" });
  city->set_editable(true);
  city->set_selected_index(0);

  auto size = make_component<ComboBox>();
  for (auto i = 8; i <= 24; i += 2) {
    size->add_item(std::to_string(i) + " pt");
  }
  size->set_maximum_row_count(5);
  size->set_selected_index(0);

  // A plain text field: caret and selection, the editing and clipboard
  // commands, Enter firing an ActionEvent.
  auto comment = make_component<TextField>("type here", 34);

  auto status = make_component<WidgetDemoTextLine>();
  auto refresh_status = [=] {
    auto text = std::string { "bold=" + widget_on_off(bold->is_selected()) + " italic=" + widget_on_off(italic->is_selected()) + " underline=" + widget_on_off(underline->is_selected()) };
    text += " | align=" + widget_selected_text(align_left) + widget_selected_text(align_center) + widget_selected_text(align_right);
    text += " | snap=" + widget_on_off(snap->is_selected()) + " guides=" + widget_on_off(guides->is_selected());
    text += " | city=" + city->get_field_text() + " size=" + size->get_selected_item();
    text += " | text=\"" + comment->get_text() + "\"";
    auto locale = Messages::get_locale();
    text += " | lang=" + (locale.empty() ? std::string { "en" } : locale);
    status->set_text(text);
  };
  auto notify_all = [refresh_status](auto const &component) {
    component->add_listener([refresh_status](ActionEvent &) {
      refresh_status();
    });
  };
  notify_all(bold);
  notify_all(italic);
  notify_all(underline);
  notify_all(align_left);
  notify_all(align_center);
  notify_all(align_right);
  notify_all(snap);
  notify_all(guides);
  notify_all(switch_guides);
  notify_all(city);
  notify_all(size);
  notify_all(comment);

  // The text field's edits (typing, deletion, paste) update the status line
  // live, through its ChangeEvent (the way a Swing DocumentListener would).
  comment->add_listener([refresh_status](ChangeEvent &) {
    refresh_status();
  });

  // ---- the dialogs ----
  //
  // A Dialog is a window of its own: it packs itself to its content, floats
  // over the frame with the theme's dialog shadow, and -- when it is modal --
  // holds the input of the windows it stands over (the mouse and the keyboard
  // alike, the focus included) until it is closed. set_visible(true) on a
  // modal dialog does not return while it is up: the screen pumps the events
  // in a nested loop, the way Swing's modal dialogs keep the dispatch thread
  // working, so a dialog can be opened from a listener and the code after the
  // call runs once it has been dismissed.

  // The modal editor of the comment line: OK applies the dialog's field to the
  // frame's, Cancel and Escape leave it as it is. Enter clicks the default
  // button (the root pane's binding, Swing's dialog submit).
  auto edit_dialog = make_component<Dialog>(frame, "Edit comment", Dialog::ModalityType::APPLICATION_MODAL);
  auto edit_content = make_component<Panel>();
  edit_content->set_layout(std::make_shared<BoxLayout>(edit_content.get(), BoxLayout::Y));
  edit_content->set_border(std::make_shared<EmptyBorder>(0, 1, 1, 1));
  edit_content->add(make_component<WidgetDemoTextLine>("The comment line:"));
  auto edit_field = make_component<TextField>("", 34);
  edit_content->add(edit_field);
  auto edit_buttons = make_component<WidgetDemoRow>();
  auto edit_ok = make_component<Button>("OK");
  auto edit_cancel = make_component<Button>("Cancel");
  edit_buttons->add(edit_ok);
  edit_buttons->add(edit_cancel);
  edit_content->add(edit_buttons);
  edit_dialog->add(edit_content);
  edit_dialog->get_root_pane()->set_default_dutton(edit_ok);

  edit_ok->add_listener([comment, edit_field, edit_dialog](ActionEvent &) {
    comment->set_text(edit_field->get_text());
    edit_dialog->set_visible(false);
  });
  edit_cancel->add_listener([edit_dialog](ActionEvent &) {
    edit_dialog->set_visible(false);
  });
  edit_dialog->add_listener([edit_dialog](KeyEvent &e) {
    if (e.id == KeyEvent::KEY_PRESSED and e.get_key_code() == KeyEvent::VK_ESCAPE) {
      edit_dialog->set_visible(false);
      e.consume();
    }
  });

  // The modeless dialog: the frame keeps working while it is up, and the two
  // "Show guides" check boxes drive the same state (the dialog's click clicks
  // the frame's, which refreshes the status line; the frame's mirrors into the
  // dialog's box).
  auto guides_dialog = make_component<Dialog>(frame, "Guides", Dialog::ModalityType::MODELESS);
  auto guides_content = make_component<Panel>();
  guides_content->set_layout(std::make_shared<BoxLayout>(guides_content.get(), BoxLayout::Y));
  guides_content->set_border(std::make_shared<EmptyBorder>(0, 1, 1, 1));
  guides_content->add(make_component<WidgetDemoTextLine>("The dialog and the frame stay live:"));
  auto dialog_guides = make_component<CheckBox>("Show guides");
  guides_content->add(dialog_guides);
  auto guides_close = make_component<Button>("Close");
  guides_content->add(guides_close);
  guides_dialog->add(guides_content);

  dialog_guides->add_listener([guides](ActionEvent &) {
    guides->do_click(std::chrono::milliseconds::zero());
  });
  guides->add_listener([guides, dialog_guides](ActionEvent &) {
    dialog_guides->set_selected(guides->is_selected());
  });
  guides_close->add_listener([guides_dialog](ActionEvent &) {
    guides_dialog->set_visible(false);
  });

  // Opening the modal dialog: the field starts on the comment line's text,
  // the dialog centers over its owner and packs to its content (a dialog
  // without a size of its own packs itself on show), and the call below
  // returns only after OK, Cancel or Escape has taken it down.
  auto show_edit_comment = [edit_dialog, edit_field, comment, frame] {
    edit_field->set_text(comment->get_text());
    edit_field->select_all();
    edit_dialog->pack();
    edit_dialog->set_location_relative_to(frame);
    edit_dialog->set_visible(true);
  };
  auto show_guides = [guides_dialog, dialog_guides, guides, frame] {
    dialog_guides->set_selected(guides->is_selected());
    guides_dialog->pack();
    guides_dialog->set_location_relative_to(frame);
    guides_dialog->set_visible(true); // a modeless dialog returns at once
  };

  // ---- the menu bar: check and radio menu items, and a submenu ----
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  auto quit = make_component<MenuItem>("Quit", Char { 'x' });
  quit->add_listener([](ActionEvent &) {
    terminal.shutdown();
  });
  file_menu->add(quit);

  auto view_menu = make_component<Menu>("View");
  view_menu->set_mnemonic('V');
  auto wrap = make_component<CheckBoxMenuItem>("Word wrap");
  wrap->add_listener([refresh_status](ActionEvent &) {
    refresh_status();
  });
  view_menu->add(wrap);
  view_menu->add_separator();

  // The density radio group hangs under a submenu of the View popup: a
  // nested Menu is a row of the popup that opens its own popup (Swing's
  // JMenu in a JPopupMenu), the radio items stay exclusive as before.
  auto density_menu = make_component<Menu>("Density");
  auto density_group = std::make_shared<ButtonGroup>();
  auto density = std::vector<std::shared_ptr<RadioButtonMenuItem>> { };
  for (auto &&label : { "Compact", "Comfortable", "Roomier" }) {
    auto item = make_component<RadioButtonMenuItem>(label);
    density_group->add(item);
    item->add_listener([refresh_status](ActionEvent &) {
      refresh_status();
    });
    density_menu->add(item);
    density.push_back(item);
  }
  density[0]->set_selected(true);
  view_menu->add(density_menu);

  // The dialogs demo: the same two dialogs the context menu offers.
  auto dialogs_menu = make_component<Menu>("Dialogs");
  dialogs_menu->set_mnemonic('D');
  auto modal_item = make_component<MenuItem>("Modal Dialog...");
  modal_item->add_listener([show_edit_comment](ActionEvent &) {
    show_edit_comment();
  });
  dialogs_menu->add(modal_item);
  auto modeless_item = make_component<MenuItem>("Modeless Dialog...");
  modeless_item->add_listener([show_guides](ActionEvent &) {
    show_guides();
  });
  dialogs_menu->add(modeless_item);

  // ---- the language menu ----
  //
  // The toolkit translates the texts it draws (see Messages): a widget's own
  // text IS the key of the lookup, so every row above already reads "File",
  // "Bold" and so on in whatever locale is current. The look-and-feel does the
  // lookup when it measures and paints, so switching the locale drops the
  // layouts of the live windows and repaints them -- labels and sizes follow.
  // The language names are proper nouns and stay as they are: the client
  // property opts them out of the lookup.
  auto language_menu = make_component<Menu>("Language");
  language_menu->set_mnemonic('L');
  auto language_group = std::make_shared<ButtonGroup>();
  auto add_language = [&](std::string const &name, std::string const &locale) {
    auto item = make_component<RadioButtonMenuItem>(name);
    item->set_client_property(Messages::TRANSLATABLE_PROPERTY, false);
    item->add_listener([locale, refresh_status](ActionEvent &) {
      Messages::set_locale(locale);
      refresh_status();
    });
    item->set_selected(Messages::get_locale() == locale);
    language_group->add(item);
    language_menu->add(item);
  };
  add_language("English", "");
  add_language("Deutsch", "de");
  add_language("Français", "fr");
  add_language("Русский", "ru");
  add_language("中文", "zh");

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(view_menu);
  menu_bar->add(dialogs_menu);
  menu_bar->add(language_menu);
  frame->set_menu_bar(menu_bar);

  // Demo-style popup wiring (see the MenuBar demo): a click on a top-level
  // menu opens its popup and closes the others; a click on the content area
  // dismisses an open popup.
  auto menus = std::vector<std::shared_ptr<Menu>> { file_menu, view_menu, dialogs_menu, language_menu };
  for (auto &&menu : menus) {
    auto weak_self = std::weak_ptr<Menu> { menu };
    auto weak_others = std::vector<std::weak_ptr<Menu>> { };
    for (auto &&other : menus) {
      if (other != menu) {
        weak_others.emplace_back(other);
      }
    }
    menu->add_listener([weak_self, weak_others](MousePressEvent &e) {
      if (e.id != MousePressEvent::MOUSE_RELEASED) {
        return;
      }
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      if (self->is_popup_menu_visible()) {
        self->set_popup_menu_visible(false);
      } else {
        for (auto &&weak_other : weak_others) {
          if (auto other = weak_other.lock()) {
            other->set_popup_menu_visible(false);
          }
        }
        self->set_popup_menu_visible(true);
      }
      self->set_armed(true);
    });
  }

  // ---- the control panel ----
  auto content = frame->get_content_pane();
  content->set_layout(std::make_shared<BorderLayout>());

  auto panel = make_component<Panel>();
  panel->set_layout(std::make_shared<BoxLayout>(panel.get(), BoxLayout::Y));

  auto heading = make_component<WidgetDemoTextLine>("Text style (toggle button, check boxes):");
  auto row1 = make_component<WidgetDemoRow>();
  row1->add(bold);
  row1->add(italic);
  row1->add(underline);
  panel->add(heading);
  panel->add(row1);

  auto heading2 = make_component<WidgetDemoTextLine>("Alignment (radio buttons in one group):");
  auto row2 = make_component<WidgetDemoRow>();
  row2->add(align_left);
  row2->add(align_center);
  row2->add(align_right);
  panel->add(heading2);
  panel->add(row2);

  auto heading3 = make_component<WidgetDemoTextLine>("Snapping (grouped check boxes):");
  auto row3 = make_component<WidgetDemoRow>();
  row3->add(snap);
  row3->add(guides);
  panel->add(heading3);
  panel->add(row3);

  auto heading4 = make_component<WidgetDemoTextLine>("Combo boxes (editable lookup / dropdown):");
  auto row4 = make_component<WidgetDemoRow>();
  row4->add(city);
  row4->add(size);
  panel->add(heading4);
  panel->add(row4);

  // A plain text field section (see the control set above).
  auto heading5 = make_component<WidgetDemoTextLine>("Text field (caret, selection, clipboard, context menu):");
  auto row5 = make_component<WidgetDemoRow>();
  row5->add(comment);
  panel->add(heading5);
  panel->add(row5);

  // The switch and a button with the optional drop shadow. The shadow's room
  // is part of the button's box (see ButtonBorder), so the layout leaves it
  // the cells it takes; the theme defines no button shadow, a program sets one
  // where it wants it.
  auto reset = make_component<Button>("Reset");
  reset->set_shadow(Shadow { BLACK_COLOR, 0.5, Point { 2, 1 } });
  reset->add_listener([=](ActionEvent &) {
    bold->set_selected(false);
    italic->set_selected(false);
    underline->set_selected(false);
    snap->set_selected(false);
    guides->set_selected(false);
    switch_guides->set_selected(false);
    refresh_status();
  });

  auto heading6 = make_component<WidgetDemoTextLine>("Switch (rounded track) and a shadowed button:");
  auto row6 = make_component<WidgetDemoRow>();
  row6->add(switch_guides);
  row6->add(reset);
  panel->add(heading6);
  panel->add(row6);

  // ---- the demo's own context menu ----
  //
  // A component's context menu (Swing's componentPopupMenu) opens on the popup
  // trigger -- a right-button press, or Shift+F10 for the keyboard (at the
  // caret of a text component) -- right over the pointer. This one carries the
  // whole row set: a check box row, plain rows, separators, a submenu of radio
  // rows and the rows that open the dialogs.
  auto context_menu = make_component<PopupMenu>();
  auto context_bold = make_component<CheckBoxMenuItem>("Bold");
  context_bold->add_listener([bold](ActionEvent &) {
    bold->do_click(std::chrono::milliseconds::zero());
  });
  context_menu->add(context_bold);
  context_menu->add_separator();

  auto context_select_all = context_menu->add("Select All in Comment");
  context_select_all->add_listener([comment](ActionEvent &) {
    comment->select_all();
  });
  auto context_clear = context_menu->add("Clear Comment");
  context_clear->add_listener([comment](ActionEvent &) {
    comment->set_text("");
  });
  context_menu->add_separator();

  // A submenu row (Swing's JMenu inside a JPopupMenu): its radio rows are
  // exclusive through a group of their own, exactly like the frame's
  // alignment radio buttons, and each row clicks its counterpart on the frame.
  auto context_align = make_component<Menu>("Alignment");
  auto context_align_group = std::make_shared<ButtonGroup>();
  auto context_align_left = make_component<RadioButtonMenuItem>("Align Left");
  auto context_align_center = make_component<RadioButtonMenuItem>("Align Center");
  auto context_align_right = make_component<RadioButtonMenuItem>("Align Right");
  context_align_group->add(context_align_left);
  context_align_group->add(context_align_center);
  context_align_group->add(context_align_right);
  context_align_left->add_listener([align_left](ActionEvent &) {
    align_left->do_click(std::chrono::milliseconds::zero());
  });
  context_align_center->add_listener([align_center](ActionEvent &) {
    align_center->do_click(std::chrono::milliseconds::zero());
  });
  context_align_right->add_listener([align_right](ActionEvent &) {
    align_right->do_click(std::chrono::milliseconds::zero());
  });
  context_align->add(context_align_left);
  context_align->add(context_align_center);
  context_align->add(context_align_right);
  context_menu->add(context_align);
  context_menu->add_separator();

  auto context_modal = context_menu->add("Modal Dialog...");
  context_modal->add_listener([show_edit_comment](ActionEvent &) {
    show_edit_comment();
  });
  auto context_modeless = context_menu->add("Modeless Dialog...");
  context_modeless->add_listener([show_guides](ActionEvent &) {
    show_guides();
  });

  // The rows show the state the frame's controls are in when the menu opens
  // (Swing's popup menus follow their actions the same way), so the check box
  // row and the radio rows never disagree with the frame.
  context_menu->add_listener([=](PopupMenuEvent &e) {
    if (e.id == PopupMenuEvent::BECOMES_VISIBLE) {
      context_bold->set_selected(bold->is_selected());
      context_align_left->set_selected(align_left->is_selected());
      context_align_center->set_selected(align_center->is_selected());
      context_align_right->set_selected(align_right->is_selected());
    }
  });

  // The panel carries the menu, and the headings inherit it (Swing's
  // setInheritsPopupMenu), so a right click anywhere on them opens it too. The
  // text field keeps the standard text menu it carries of its own, and the
  // buttons and combo boxes keep their own gestures: the popup trigger is not
  // a press of theirs.
  panel->set_component_popup_menu(context_menu);
  for (auto &&heading : { heading, heading2, heading3, heading4, heading5 }) {
    heading->set_inherits_popup_menu(true);
  }

  content->add(panel, BorderLayout::CENTER);
  content->add(status, BorderLayout::SOUTH);

  // A click outside an open popup dismisses it.
  content->add_listener([menus](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    for (auto &&menu : menus) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
  });

  refresh_status();
  terminal.set_title("tui++ widget demo - " + type);
  frame->set_visible(true);
  terminal.run_event_loop();
}
