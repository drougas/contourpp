#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

//#define CONTOURPP_DEBUG_HID_COMM 1

#ifdef CONTOURPP_DEBUG_HID_COMM
#include <iostream>
#endif

#ifdef CONTOURPP_USE_LIBHID
#define HAVE_STDBOOL_H 1
#include <hid.h>
#else
#include <hidapi/hidapi.h>
#endif

#include "hid_commands.hpp"

using namespace contourpp;

static const unsigned short vendor_id = 0x1A79; // Bayer
static const unsigned short device_ids[] = {
  0x6002, // Contour USB
  0x7410, // Contour Next USB
  0x7800, // Contour Next ONE
  0x0000
};
static const char     blocksize = 64;

static const char ACK = 0x06;
static const char ENQ = 0x05;
static const char EOT = 0x04;
static const char ETB = 0x17;
static const char ETX = 0x03;
static const char NAK = 0x15;
static const char STX = 0x02;
static const char CR  = '\r';
static const char LF  = '\n';

static inline void test_ret(const char* name, int ret)
{
#ifdef CONTOURPP_USE_LIBHID
  if (ret != 0)
#else
  if (ret < 0)
#endif
  {
    std::stringstream sstream;
    sstream << name << " failed with return code " << ret
      << " == " << std::showbase << std::hex << ret;
    throw std::runtime_error(sstream.str());
  }
}

std::string interface::to_string(const char* first, const char* last,
  const char* prefix, const char* suffix)
{
  std::stringstream sstream;
  sstream << std::hex << prefix;
  char c;
  unsigned short n;

  for (; first < last; ++first) {
    c = *first;
    if      (c ==  CR) sstream << "<CR>";
    else if (c ==  LF) sstream << "<LF>";
    else if (c == ACK) sstream << "<ACK>";
    else if (c == ENQ) sstream << "<ENQ>";
    else if (c == EOT) sstream << "<EOT>";
    else if (c == ETB) sstream << "<ETB>";
    else if (c == ETX) sstream << "<ETX>";
    else if (c == NAK) sstream << "<NAK>";
    else if (c == STX) sstream << "<STX>";
    else if (::isprint(c)) sstream << c;
    else {
      n = (unsigned char)c;
      sstream << "\\x";
      if (n < 0x10) sstream << '0';
      sstream << n;
    }
  }

  sstream << suffix;
  return sstream.str();
}

static inline void read(hid_device* hid, std::vector<char>& ret)
{
  static const char maxpayload = blocksize - 4;
  static const char uisize = (blocksize / sizeof(size_t)) + ((blocksize % sizeof(size_t)) != 0);

  size_t uibuf[uisize], *ui, *ue = uibuf + uisize;
  char* buf = (char*)uibuf;
  const char* b(buf + 4);
  char n;

  ret.clear();
  do {
    for (ui = uibuf; ui < ue; ++ui)
      *ui = 0;

#ifdef CONTOURPP_USE_LIBHID
    test_ret("hid_interrupt_read()", ::hid_interrupt_read(hid, 0x81, buf, blocksize, 5000));
#else
    test_ret("hid_read()", ::hid_read_timeout(hid, (unsigned char*)buf, blocksize, 5000));
#endif

    n = (buf[3] < maxpayload)? buf[3] : maxpayload;
    for (const char *i(b), *e(b + n); i < e; ++i)
      ret.push_back(char(*i));
  } while (n == maxpayload);

#ifdef CONTOURPP_DEBUG_HID_COMM
  std::cerr << "*** " << interface::to_string(ret.data(), ret.data() + ret.size()) << std::endl;
#endif
}

static inline void write(hid_device* const& hid, const char& c)
{
  char buf[5] = { 'A', 'B', 'C', 1, c };

#ifdef CONTOURPP_DEBUG_HID_COMM
  std::cerr << ">>> " << interface::to_string(&c, (&c) + 1) << std::endl;
#endif

#ifdef CONTOURPP_USE_LIBHID
  test_ret("hid_interrupt_write()", ::hid_interrupt_write(hid, 0x01, buf, 5, 5000));
#else
  test_ret("hid_write()", ::hid_write(hid, (unsigned char*)buf, 5));
#endif
}

// Static struct for initialization / cleanup of hidapi (or libhid).
struct HidInitializer
{
  bool initialized_;

  HidInitializer() : initialized_(false) {}
#ifdef CONTOURPP_USE_LIBHID
  ~HidInitializer() { if (initialized_) ::hid_cleanup(); }
#else
  ~HidInitializer() { if (initialized_) ::hid_exit(); }
#endif

  inline void init() {
    if (initialized_)
      return;
#if defined(CONTOURPP_USE_LIBHID) && defined(CONTOURPP_DEBUG_HID_COMM)
    ::hid_set_debug(HID_DEBUG_ALL);
    ::hid_set_debug_stream(stderr);
    ::hid_set_usb_debug(0); // passed directly to libusb
#endif
    test_ret("hid_init()", ::hid_init());
    initialized_ = true;
  }
};

static HidInitializer s_initializer;

bool interface::open()
{
  if (hid_)
    return false;

  s_initializer.init();
  data_.reserve(5 * size_t(blocksize));

#ifdef CONTOURPP_USE_LIBHID
  HIDInterfaceMatcher matcher = { vendor_id, 0x0000, NULL, NULL, 0 };
  int open_result = -1;
  if (!(hid_ = ::hid_new_HIDInterface()))
    throw std::runtime_error("hid_new_HIDInterface() failed. Not enough memory?");
  for (int i = 0; device_ids[i] && (open_result < 0); ++i) {
#ifdef CONTOURPP_DEBUG_HID_COMM
  std::cerr << "/// Searching for " << std::hex << device_ids[i] << std::dec << std::endl;
#endif
    matcher.product_id = device_ids[i];
    open_result = ::hid_force_open(hid_, 0, &matcher, 3);
  }
  test_ret("hid_force_open()", open_result);
#ifdef CONTOURPP_DEBUG_HID_COMM
  std::cerr << "/// Found " << std::hex << device_ids[i] << std::dec << std::endl;
#endif
#else
  for (int i = 0; device_ids[i] && !hid_; ++i)
    hid_ = ::hid_open(vendor_id, device_ids[i], NULL);
  if (!hid_)
    throw std::runtime_error("hid_open() failed.");
#endif

  return true;
}

bool interface::close()
{
  if (!hid_)
    return false;

#ifdef CONTOURPP_USE_LIBHID
  test_ret("hid_close()", ::hid_close(hid_));
  ::hid_delete_HIDInterface(&hid_);
#else
  ::hid_close(hid_);
#endif

  hid_ = NULL;
  state_ = establish;
  return true;
}

static inline unsigned char hex_char_to_number(char c)
{
  if ((c >= '0') && (c <= '9')) return c - '0';
  if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
  if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
  return 255;
}

bool interface::parseframe(const char*& text_begin, const char*& text_end)
{
  const char* b(data_.data());
  const char* e(b + data_.size());
  const char* i(std::find(b, e, STX));

  text_begin = text_end = NULL;
  if ((++i) >= e)
    return false; // <STX> not found

  unsigned char checksum_calc = *i, checksum_given = 0;

  { // check the record number
    const unsigned char recno = *i - '0';
    if (recno > 7)
      throw std::runtime_error(to_string(b, e, "Invalid recno in frame: \"", "\""));

    if (currecno_ > 7)
      currecno_ = recno;
    else if (recno != currecno_) {
      if (((recno + 1) & 7) == currecno_)
        return true; // retransmitted frame

      std::stringstream sstream;
      sstream << "Bad recno, got " << recno << ", expected " << currecno_ << ", frame: \"";
      throw std::runtime_error(to_string(b, e, sstream.str().c_str(), "\""));
    }
  } // check the record number

  { // fill in text and calculate checksum_calc
    text_begin = ++i;
    for (; (i < e) && (*i != CR); ++i)
      checksum_calc += static_cast<unsigned char>(*i);
    checksum_calc += CR;
    text_end = i;

    if ((i >= e) || ((++i) >= e) || ((*i != ETX) && (*i != ETB)))
      throw std::runtime_error(to_string(b, e, "Could not parse text in frame: \"", "\""));

    checksum_calc += *i;
  } // fill in text and calculate checksum_calc

  { // parse the checksum given in the frame
    unsigned char digit = ((++i) < e)? hex_char_to_number(*i) : 255;
    checksum_given = digit << 4;

    digit = ((digit < 16) && ((++i) < e))? hex_char_to_number(*i) : 255;
    checksum_given = checksum_given | digit;

    if (digit > 15)
      throw std::runtime_error(to_string(b, e, "Could not parse checksum in frame: \"", "\""));
  } // parse the checksum given in the frame

  // Compare the calculated with the given checksum.
  if (checksum_calc != checksum_given) {
    std::stringstream sstream;
    sstream << std::hex << std::showbase << "Bad checksum, got "
      << int(checksum_calc) << ", expected " << int(checksum_given) << ", frame: \"";
    throw std::runtime_error(to_string(b, e, sstream.str().c_str(), "\""));
  }

  if (((++i) >= e) || (*i != CR) || ((++i) >= e) || (*i != LF))
    throw std::runtime_error(to_string(b, e, "Could not parse <CR><LF> in frame: \"", "\""));

  currecno_ = ((currecno_ + 1) & 7);
  return true;
}


bool interface::sync(const char*& result_begin, const char*& result_end)
{
  result_begin = result_end = NULL;
  if (state_ == establish)
    write(hid_, ENQ);
  else if (state_ != data)
    return false;

  do {
    result_begin = result_end = NULL;
    read(hid_, data_);

    if (state_ == establish) {
      if (data_.back() == NAK) { // got a <NAK>, send <EOT>
        write(hid_, foo_);
        ++foo_;
      }
      else if (data_.back() == ENQ) { // got an <ENQ>, send <ACK>
        write(hid_, ACK);
        currecno_ = 8;
      }
    }
    else if (data_.back() == EOT) { // got an <EOT>, done
      state_ = precommand;
      return false;
    }

    bool parsed = false;
    try {
      parsed = parseframe(result_begin, result_end);
    }
    catch (...) {
      // Got something we don't understand, <NAK> it
      write(hid_, NAK);
      throw;
    }

    if (parsed) { // parsed frame, send ACK
      write(hid_, ACK);
      state_ = data;
      if (result_begin != NULL) { // Message Terminator Record frame received, done
        if (result_begin[0] == 'L') { 
          return false;
        }
      }
    }
    else
      write(hid_, NAK);
  } while (result_begin >= result_end);

  return true;
}

bool interface::ensurecommand()
{
  if (!hid_)
    return false;

  if (state_ == establish || state_ == data) {
    do {
      write(hid_, NAK);
      read(hid_, data_);
    } while (data_.empty() || data_.back() != EOT);
    state_ = precommand;
  }

  if (state_ == precommand) {
    do {
      write(hid_, ENQ);
      read(hid_, data_);
    } while (data_.empty() || data_.back() != ACK);
    state_ = command;
  }

  return (state_ == command);
}

const std::vector<char>& interface::send_command(char c)
{
  // Enter remote command mode if needed

  static const std::vector<char> empty_vector;

  if (!ensurecommand())
    return empty_vector;

  write(hid_, c);
  read(hid_, data_);

  if (data_.empty() || data_.back() != ACK)
    return empty_vector;

  data_.pop_back();
  return data_;
}
