#include "Selector.h"
#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel (ptr...
#include <qlist.h>               // for QList, QList<>::iterator
class QObject;

Selector::Selector(QAbstractItemModel *model_pointer, QObject* parent_pointer) : 
    QItemSelectionModel(model_pointer, parent_pointer) { }

void Selector::select(const QItemSelection &new_selection, QItemSelectionModel::SelectionFlags command) {
    auto new_indices = new_selection.indexes();
    if (!(new_indices.isEmpty()) && command.testAnyFlags(QItemSelectionModel::Select | QItemSelectionModel::Toggle)) {
        auto previous_indices = selection().indexes();
        QModelIndex first_selected = new_indices.at(0);
        QModelIndex selection_parent;
        if (command.testFlag(QItemSelectionModel::Clear) || previous_indices.isEmpty()) {
            selection_parent = first_selected.parent();
        } else {
            selection_parent = previous_indices.at(0).parent();
            auto is_valid = first_selected.parent() == selection_parent; 
            if (!(is_valid)) {
                for (auto index : new_indices) {
                    is_valid = index.parent() == selection_parent;
                    if (is_valid) {
                        first_selected = index;
                        break;
                    }
                }
                if (!(is_valid)) {
                    return;
                }  
            }   
        }
        QModelIndex last_selected = first_selected;
        for (auto index : new_indices) {
            if (index.parent() == selection_parent) {
                last_selected = index;
            }
        }
        return QItemSelectionModel::select(QItemSelection(first_selected, last_selected), command);
    }
    return QItemSelectionModel::select(new_selection, command);
}