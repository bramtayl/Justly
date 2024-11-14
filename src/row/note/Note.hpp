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

// A subnote should have the following method:
// static auto get_note_type() -> const char*;
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

template <std::derived_from<Note> SubNote>
void add_note_location(QTextStream &stream, int chord_number, int note_number)  {
  stream << QObject::tr(" for chord ") << chord_number + 1 <<
         QObject::tr(SubNote::get_note_type()) << note_number + 1;
};

template <std::derived_from<Note> SubNote, std::derived_from<Named> SubNamed>
auto substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                          const SubNamed *current_sub_named_pointer,
                          const char *default_one, int chord_number,
                          int note_number,
                          const char *missing_title,
                          const char *missing_message,
                          const char *default_message) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr(missing_message);
    add_note_location<SubNote>(stream, chord_number, note_number);
    stream << QObject::tr(default_message);
    QMessageBox::warning(&parent, QObject::tr(missing_title), message);
    sub_named_pointer = &get_by_name<SubNamed>(default_one);
  }
  return *sub_named_pointer;
}
