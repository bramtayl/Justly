#include <QtCore/QtGlobal>

#if defined(JUSTLY_LIBRARY)
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

#define NO_MOVE_COPY(class_name)                          \
  class_name(const class_name&) = delete;                 \
  auto operator=(const class_name&)->class_name = delete; \
  class_name(class_name&&) = delete;                      \
  auto operator=(class_name&&)->class_name = delete;
