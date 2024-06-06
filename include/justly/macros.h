#define NO_MOVE_COPY(type_name)                          \
  type_name(const type_name &) = delete;                 \
  auto operator=(const type_name &)->type_name = delete; \
  type_name(type_name &&) = delete;                      \
  auto operator=(type_name &&)->type_name = delete;