#pragma once

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <concepts>
#include <nlohmann/json.hpp>

#include "named/Named.hpp"
#include "row/Row.hpp"

class QWidget;
struct Player;
struct Program;

struct Note : Row {
  Note() = default;
  explicit Note(const nlohmann::json &json_note);

  [[nodiscard]] virtual auto
  get_closest_midi(Player &player, int channel_number, int chord_number,
                   int note_number) const -> short = 0;

  [[nodiscard]] virtual auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & = 0;
};

void add_note_location(QTextStream &stream, int chord_number, int note_number,
                       const char *note_type);

template <std::derived_from<Named> SubNamed>
auto substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                          const SubNamed *current_sub_named_pointer,
                          int chord_number, int note_number,
                          const char *note_type) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    const char *default_one = SubNamed::get_default();

    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("No ") << QObject::tr(SubNamed::get_type_name());
    add_note_location(stream, chord_number, note_number, note_type);
    stream << QObject::tr(". Using ") << QObject::tr(default_one) << ".";
    QMessageBox::warning(&parent, QObject::tr(SubNamed::get_missing_error()),
                         message);
    sub_named_pointer = &get_by_name<SubNamed>(default_one);
  }
  return *sub_named_pointer;
}
