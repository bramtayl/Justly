#include <QtCore/QtGlobal>

#if defined(JUSTLY_LIBRARY)
#  define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#  define JUSTLY_EXPORT Q_DECL_IMPORT
#endif