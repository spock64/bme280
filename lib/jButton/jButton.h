// *** BUTTON ***

typedef void (*bFunc)();

void btnSetHandler(
  int clicks,
  const char * name,
  bFunc f
);
void doButton();
void setupButton(int io_pin);
bool buttonDown();
