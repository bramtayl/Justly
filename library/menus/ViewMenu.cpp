#include "menus/ViewMenu.hpp"

#include <QtGui/QKeySequence>

#include "widgets/SongWidget.hpp"

ViewMenu::ViewMenu()
    : QMenu(ViewMenu::tr("&View")),
      back_to_chords_action(ViewMenu::tr("&Back to chords")),
      edit_pitched_voices_action(ViewMenu::tr("&Pitched voices")),
      edit_unpitched_voices_action(ViewMenu::tr("&Unpitched voices")),
      previous_chord_action(ViewMenu::tr("&Previous chord")),
      next_chord_action(ViewMenu::tr("&Next chord")),
      show_piano_roll_action(ViewMenu::tr("&Piano roll")),
      zoom_in_action(ViewMenu::tr("Zoom &in")),
      zoom_out_action(ViewMenu::tr("Zoom &out")) {
  add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);
  add_menu_action(*this, edit_pitched_voices_action, QKeySequence::UnknownKey, true);
  add_menu_action(*this, edit_unpitched_voices_action, QKeySequence::UnknownKey, true);

  add_menu_action(*this, previous_chord_action, QKeySequence::PreviousChild,
                  false);
  add_menu_action(*this, next_chord_action, QKeySequence::NextChild, false);

  add_menu_action(*this, show_piano_roll_action, QKeySequence::UnknownKey,
                  true);
  show_piano_roll_action.setCheckable(true);

  add_menu_action(*this, zoom_in_action, QKeySequence::ZoomIn, true);
  add_menu_action(*this, zoom_out_action, QKeySequence::ZoomOut, true);
}
