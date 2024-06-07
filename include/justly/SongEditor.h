#pragma once

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qabstractitemmodel.h>   // for QModelIndex (ptr only), QAbstractIte...
#include <qabstractitemview.h>    // for QAbstractItemView
#include <qaction.h>              // for QAction
#include <qbytearray.h>           // for QByteArray
#include <qclipboard.h>           // for QClipboard
#include <qcombobox.h>            // for QComboBox
#include <qcontainerfwd.h>        // for QStringList
#include <qdir.h>                 // for QDir
#include <qdockwidget.h>          // for QDockWidget, QDockWidget::NoDoc...
#include <qfiledialog.h>          // for QFileDialog, QFileDialog::Accep...
#include <qformlayout.h>          // for QFormLayout
#include <qframe.h>               // for QFrame
#include <qguiapplication.h>      // for QGuiApplication
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelec...
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlist.h>                // for QList, QList<>::iterator
#include <qmainwindow.h>          // for QMainWindow
#include <qmenu.h>                // for QMenu
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>          // for QMessageBox, QMessageBox::Yes
#include <qmimedata.h>            // for QMimeData
#include <qnamespace.h>           // for LeftDockWidgetArea, WindowFlags
#include <qrect.h>                // for QRect
#include <qscreen.h>              // for QScreen
#include <qsize.h>                // for QSize
#include <qsizepolicy.h>          // for QSizePolicy, QSizePolicy::Fixed
#include <qspinbox.h>             // for QDoubleSpinBox
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths:...
#include <qstring.h>              // for QString
#include <qtmetamacros.h>         // for Q_OBJECT
#include <qundostack.h>           // for QUndoStack
#include <qvariant.h>             // for QVariant
#include <qwidget.h>              // for QWidget

#include <cstddef>                // for size_t
#include <fstream>                // for ofstream, ifstream, ostream
#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __unique_ptr_t
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::parse...
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <utility>                // for move
#include <vector>                 // for vector

#include "justly/Chord.h"  // for Chord
#include "justly/Song.h"   // for Song, MAX_STARTING_KEY, MAX_STA...
#include "justly/SongEditor.h"
#include "justly/StartingField.h"     // for starting_instrument_id, startin...
#include "justly/TreeLevel.h"         // for TreeLevel
#include "src/ChordsModel.h"          // for ChordsModel
#include "src/ChordsView.h"           // for ChordsView
#include "src/Instrument.h"           // for Instrument
#include "src/InstrumentEditor.h"     // for InstrumentEditor
#include "src/JsonErrorHandler.h"     // for JsonErrorHandler
#include "src/Player.h"               // for Player
#include "src/StartingValueChange.h"  // for StartingValueChange

class ChordsModel;
class InstrumentEditor;
class Player;
class QAbstractItemView;
class QAction;
class QDoubleSpinBox;
class QItemSelection;
class QUndoStack;
class QWidget;

class SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;

  QDoubleSpinBox* starting_tempo_editor_pointer;
  QDoubleSpinBox* starting_volume_editor_pointer;
  QDoubleSpinBox* starting_key_editor_pointer;
  InstrumentEditor* starting_instrument_editor_pointer;

  QAction* insert_before_action_pointer;
  QAction* insert_after_action_pointer;
  QAction* insert_into_action_pointer;
  QAction* remove_action_pointer;
  QAction* copy_action_pointer;
  QAction* paste_before_action_pointer;
  QAction* paste_after_action_pointer;
  QAction* paste_into_action_pointer;
  QAction* save_action_pointer;
  QAction* play_action_pointer;

  QAbstractItemView* chords_view_pointer;

  QUndoStack* undo_stack_pointer;

  ChordsModel* chords_model_pointer;

  QString current_file;
  QString current_folder;

  std::unique_ptr<Player> player_pointer;

  TreeLevel copy_level;

  void export_recording();
  void open();
  void save_as();

  void set_starting_instrument(int);
  void set_starting_value(StartingField, const QVariant&);

  void initialize_controls();

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void paste(int, const QModelIndex&);

  void update_actions();

 public:
  inline ~SongEditor() override { undo_stack_pointer->disconnect(); }

  // prevent moving and copying;
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;

  explicit SongEditor(QWidget* = nullptr, Qt::WindowFlags = Qt::WindowFlags());

  inline void export_to(const QString& filename) {
    player_pointer->export_to(filename.toStdString());
  }

  void open_file(const QString&);
  void save_as_file(const QString&);

  void paste_text(int, const std::string&, const QModelIndex&);

  void copy_selected();
  void insert_before();
  void insert_after();
  void insert_into();

  void paste_before();
  void paste_after();
  void paste_into();

  void remove_selected();
  void play_selected() const;

  void save();

  inline void play(int first_child_number, int number_of_children,
                   const QModelIndex& parent_index) const {
    player_pointer->play_selection(
        first_child_number, number_of_children,
        chords_model_pointer->get_chord_number(parent_index));
  }

  void stop() const;

  void undo();
  void redo();

  [[nodiscard]] inline auto get_current_file() const -> const QString& {
    return current_file;
  }

  inline void set_starting_control(StartingField value_type,
                                   const QVariant& new_value, bool no_signals) {
    switch (value_type) {
      case starting_key_id: {
        auto new_double = new_value.toDouble();
        if (starting_key_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_key_editor_pointer->blockSignals(true);
          }
          starting_key_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_key_editor_pointer->blockSignals(false);
          }
        }
        song.starting_key = new_double;
        break;
      }
      case starting_volume_id: {
        auto new_double = new_value.toDouble();
        if (starting_volume_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_volume_editor_pointer->blockSignals(true);
          }
          starting_volume_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_volume_editor_pointer->blockSignals(false);
          }
        }
        song.starting_volume = new_double;
        break;
      }
      case starting_tempo_id: {
        auto new_double = new_value.toDouble();
        if (starting_tempo_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_tempo_editor_pointer->blockSignals(true);
          }
          starting_tempo_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_tempo_editor_pointer->blockSignals(false);
          }
        }
        song.starting_tempo = new_double;
        break;
      }
      case starting_instrument_id:
        if (starting_instrument_editor_pointer->get_instrument_pointer() !=
            new_value.value<const Instrument*>()) {
          if (no_signals) {
            starting_instrument_editor_pointer->blockSignals(true);
          }
          starting_instrument_editor_pointer->set_instrument_pointer(
              new_value.value<const Instrument*>());
          if (no_signals) {
            starting_instrument_editor_pointer->blockSignals(false);
          }
        }
        song.starting_instrument_pointer = new_value.value<const Instrument*>();
        break;
      default:
        break;
    }
  }

  [[nodiscard]] inline auto get_selected_rows() const -> QModelIndexList {
    return chords_view_pointer->selectionModel()->selectedRows();
  }

  [[nodiscard]] inline auto get_starting_value(StartingField value_type) const
      -> QVariant {
    switch (value_type) {
      case starting_key_id:
        return QVariant::fromValue(song.starting_key);
      case starting_volume_id:
        return QVariant::fromValue(song.starting_volume);
      case starting_tempo_id:
        return QVariant::fromValue(song.starting_tempo);
      case starting_instrument_id:
        return QVariant::fromValue(song.starting_instrument_pointer);
      default:
        return {};
    }
  }

  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
};
