#include "percussion_set/PercussionSetEditor.hpp"

#include <QStringListModel>

#include "named/Named.hpp"
#include "percussion_set/PercussionSet.hpp"

PercussionSetEditor::PercussionSetEditor(QWidget *parent_pointer_input)
    : NamedEditor<PercussionSet>(get_all_percussion_sets(),
                                 parent_pointer_input) {
  static auto percussion_sets_model = get_list_model(get_all_percussion_sets());
  setModel(&percussion_sets_model);
};