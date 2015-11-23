#ifndef HID_COMMANDS_H__
#define HID_COMMANDS_H__

#include <string>
#include <vector>

#ifdef CONTOURPP_USE_LIBHID
// Use libhid - untested
struct HIDInterface_t;
typedef HIDInterface_t hid_device;
#else
// Use hidapi
extern "C" {
  struct hid_device_;
  typedef struct hid_device_ hid_device; /**< opaque hidapi structure */
}
#endif

namespace contourpp
{

class interface
{
public:
  static std::string to_string(const char* first, const char* last,
      const char* prefix = "", const char* suffix = "");

private:
  enum State { establish = 0, data = 1, precommand = 2, command = 3 };

  std::vector<char> data_;
  hid_device* hid_;
  State state_;
  char foo_;
  unsigned char currecno_;

  bool parseframe(const char*& text_begin, const char*& text_end);
  bool ensurecommand();

  // Copy not allowed
  interface(const interface&);
  interface & operator=(const interface&);

public:

  interface(bool const& doOpen = true)
    : hid_(NULL), state_(establish), foo_(0), currecno_(8)
  {
    if (doOpen)
      open();
  }

  ~interface() { close(); }

  inline bool is_open() const { return hid_ != NULL; }
  bool open();
  bool close();

  // Sync with meter and yield received data frames.
  bool sync(const char*& result_begin, const char*& result_end);

  // Send a command to the meter
  const std::vector<char>& send_command(char c);
};

} // namespace contourpp

#endif // HID_COMMANDS_H__
