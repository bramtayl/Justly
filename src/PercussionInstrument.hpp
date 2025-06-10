#pragma once

#include "Program.hpp"

static const auto TAMBOURINE_MIDI = 54;

struct PercussionInstrument {
  const Program *percussion_set_pointer;
  short midi_number;

  explicit PercussionInstrument(
      const Program *const percussion_set_pointer_input = nullptr,
      const short midi_number_input = TAMBOURINE_MIDI)
      : percussion_set_pointer(percussion_set_pointer_input),
        midi_number(midi_number_input){};

  [[nodiscard]] auto
  operator==(const PercussionInstrument &other_percussion_instrument) const {
    return percussion_set_pointer ==
               other_percussion_instrument.percussion_set_pointer &&
           midi_number == other_percussion_instrument.midi_number;
  }
};

Q_DECLARE_METATYPE(PercussionInstrument);

[[nodiscard]] static auto percussion_instrument_is_default(
    const PercussionInstrument &percussion_instrument) {
  return percussion_instrument.percussion_set_pointer == nullptr;
}

static inline void maybe_set_xml_percussion_instrument(
    xmlNode &node, const char *const field_name,
    const PercussionInstrument &percussion_instrument) {
  if (!(percussion_instrument_is_default(percussion_instrument))) {
    auto &percussion_instrument_node = get_new_child(node, field_name);
    set_xml_string(percussion_instrument_node, "percussion_set",
                   get_reference(percussion_instrument.percussion_set_pointer)
                       .original_name);
    set_xml_int(percussion_instrument_node, "midi_number",
                percussion_instrument.midi_number);
  }
}

static inline void
set_percussion_instrument_from_xml(PercussionInstrument &percussion_instrument,
                                   xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while ((field_pointer != nullptr)) {
    auto &field_node = get_reference(field_pointer);
    const auto &name = get_xml_name(field_node);
    if (name == "percussion_set") {
      percussion_instrument.percussion_set_pointer =
          &xml_to_program(get_percussion_sets(), field_node);
    } else if (name == "midi_number") {
      percussion_instrument.midi_number =
          static_cast<short>(xml_to_int(field_node));
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}
