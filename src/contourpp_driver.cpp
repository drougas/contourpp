#include <algorithm>
#include <string>
#include <vector>
#include <istream>
#include <iostream>
#include "contourpp_driver.hpp"
#include "hid_commands.hpp"

//referencemap['B'] = "whole blood";
//referencemap['P'] = "plasma";
//referencemap['C'] = "capillary";
//referencemap['D'] = "deproteinized whole blood";

static const std::string bayer_types[] = {
  "^^^Glucose",
  "^^^Insulin",
  "^^^Insulin",
  "^^^Carb",
  "^^^Unknown"
};

static const std::string bayer_types2[] = { "mg/dL^P", "1^", "2^", "1^", "" };

static bool equals(const std::string& s, const char* b, const char* e)
{
  const size_t n = e - b;
  if (s.size() != n)
    return false;

  for (size_t i = 0; i < n; ++i)
    if (::tolower(b[i]) != ::tolower(s[i]))
      return false;

  return true;
}

static const unsigned char bayer_type_idx_unknown =
  (sizeof(bayer_types) / sizeof(bayer_types[0])) - 1;

const char* contourpp::record::get_bayer_type() const
{
  unsigned char tidx = bayer_type_idx_unknown;
  if (value_)
    tidx = is_glucose()? 0 : (1 + tag2_);
  return bayer_types[tidx].c_str();
}

const char* contourpp::record::get_bayer_type2() const
{
  unsigned char tidx = bayer_type_idx_unknown;
  if (value_)
    tidx = is_glucose()? 0 : (1 + tag2_);
  return bayer_types2[tidx].c_str();
}

bool contourpp::record::parse_bayer(const char* b, const char* e, char field_sep)
{
  clear();

  if ((e - b < 2) || (*b != field_sep))
    return false;

  for (++b; b < e && *b != field_sep; ++b) {
    if ((*b < '0') || (*b > '9')) return false;
    index_ = (index_ * 10) + (*b - '0');
  }
  if (b >= e)
    return false;

  const char* type_end = std::find(++b, e, field_sep);
  unsigned char tidx = 0;
  if (type_end >= e)
    return false;

  while ((tidx < bayer_type_idx_unknown) && !equals(bayer_types[tidx], b, type_end))
    ++tidx;

  for (b = type_end + 1; (b < e) && (*b != field_sep); ++b) {
    if ((*b < '0') || (*b > '9')) return false;
    value_ = (value_ * 10) + (*b - '0');
  }
  if (b >= e)
    return false;

  type_end = std::find(++b, e, field_sep);
  if (type_end >= e)
    return false;

  if (1 == tidx) // Insulin - Short or long acting?
    tidx += equals(bayer_types2[2], b, type_end);

  if ((b = std::find(++type_end, e, field_sep)) >= e)
    return false;

  if (0 == tidx) { // Glucose - parse tags
    for (++b; (b < e) && (*b != field_sep); ++b) {
      switch (*b) {
        case 'C': tags_ |=  1; continue; // control
        case 'B': tags_ |=  2; continue; // before food
        case 'A': tags_ |=  4; continue; // after food
        case 'D': tags_ |=  8; continue; // don't feel right
        case 'I': tags_ |= 16; continue; // sick
        case 'S': tags_ |= 32; continue; // stress
        case 'X': tags_ |= 64; continue; // activity
        case '<': value_ =  9; continue; // result low
        case '>': value_ = 601; continue; // result high
        case '/': continue;
        case 'Z':
          tags_ |= 4; // after food
          if (++b >= e)
            return false;
          else if ((*b >= '0') && (*b <= '9'))
            tag2_ = 15 * (*b - '0');
          else if ((*b >= 'A') && (*b <= 'F'))
            tag2_ = 15 * (*b - 'A' + 10);
          else if ((*b >= 'a') && (*b <= 'f'))
            tag2_ = 15 * (*b - 'a' + 10);
          else
            return false;
      }
    }
  }
  else {
    tags_ = 128;
    tag2_ = tidx - 1;
    b = std::find(++b, e, field_sep);
  }

  if (b >= e)
    return false;

  if ((b = std::find(++b, e, field_sep)) >= e)
    return false;

  ++b;
  if (e - b < 12)
    return false;

  if (b[ 0] < '0' || b[ 1] < '0' || b[ 2] < '0' || b[ 3] < '0') return false;
  if (b[ 4] < '0' || b[ 5] < '0' || b[ 6] < '0' || b[ 7] < '0') return false;
  if (b[ 8] < '0' || b[ 9] < '0' || b[10] < '0' || b[11] < '0') return false;
  if (b[ 0] > '9' || b[ 1] > '9' || b[ 2] > '9' || b[ 3] > '9') return false;
  if (b[ 4] > '1' || b[ 5] > '9' || b[ 6] > '3' || b[ 7] > '9') return false;
  if (b[ 8] > '2' || b[ 9] > '9' || b[10] > '6' || b[11] > '9') return false;

  datetime_ = datetime_t(
    boost::gregorian::date(
      (b[ 0] - '0') * 1000 + (b[ 1] - '0') * 100 + (b[ 2] - '0') * 10 + (b[ 3] - '0'),
      (b[ 4] - '0') *   10 + (b[ 5] - '0'),        (b[ 6] - '0') * 10 + (b[ 7] - '0')
    ),
    boost::posix_time::time_duration(
      (b[ 8] - '0') *   10 + (b[ 9] - '0'), (b[10] - '0') * 10 + (b[11] - '0'), 0
    )
  );

  return true;
}

/*
static short get_digits(const char* b, const char* e, int min_digits, int max_digits = 10)
{
  if ((e - b < min_digits) || (min_digits <= 0) || (max_digits < min_digits))
    return -1;

  short ret = 0;

  while (min_digits > 0) {
    if (*b < '0' || *b > '9')
      return -1;

    ret = (10 * ret) + (*b - '0');
    ++b;
    --min_digits;
    --max_digits;
  }

  while (max_digits && (b < e) && (*b >= '0') && (*b <= '9')) {
    ret = (10 * ret) + (*b - '0');
    ++b;
    --max_digits;
  }

  return ret;
}
*/

bool contourpp::record::parse_csv(const char* b, const char* const& e, const char& field_sep)
{
  clear();

  if ((e - b < 2) || (*b != field_sep))
    return false;

  return false;
}

static inline const char* parse(const char* b, const char* const& e, std::string& s, const char& c)
{
  s.clear();
  for (; (b < e) && (*b != c); ++b)
    s.push_back(*b);
  return b;
}

static inline const char* parse(const char* b, const char* const& e, std::string& s)
{
  s.clear();
  for (; b < e; ++b)
    s.push_back(*b);
  return e;
}

bool contourpp::record_parser::parse_H(const char* b, const char* const& e)
{
  field_sep_ = *b;

  if ((++b) >= e)
    return false;

  repeat_sep_ = *b;

  if ((++b) >= e)
    return false;

  comp_sep_ = *b;

  if ((++b) >= e)
    return false;

  escape_sep_ = *b;

  if ((b += 2) >= e)
    return false;

  if ((b = std::find(b, e, field_sep_)) >= e)
    return false;

  if ((b = ::parse(++b, e, password_, field_sep_)) >= e) return false;
  if ((b = ::parse(++b, e,  product_,  comp_sep_)) >= e) return false;
  if ((b = ::parse(++b, e, versions_,  comp_sep_)) >= e) return false;
  if ((b = ::parse(++b, e,   serial_,  comp_sep_)) >= e) return false;
  if ((b = ::parse(++b, e,      sku_, field_sep_)) >= e) return false;

  if ((b = ::parse(++b, e, device_info_, field_sep_)) >= e) return false;

  result_count_ = 0;
  while (((++b) < e) && (*b != field_sep_)) {
    if ((*b < '0') || (*b > '9'))
      return false;
    result_count_ = (result_count_ * 10) + (*b - '0');
  }

  return b < e;
}

bool contourpp::record_parser::parse_P(const char* const& b, const char* const& e)
{
  return ::parse(b + 1, e, patient_info_) <= e;
}

bool contourpp::record_parser::parse(const char* const& b, const char* const& e, record& rec)
{
  bool result = false;

  if (b < e) {
    switch (*b) {
      case 'R': if (rec.parse_bayer(b + 1, e, field_sep_)) return true; break;
      case 'H': result = parse_H(b + 1, e); break;
      case 'O': result = parse_O(b + 1, e); break;
      case 'P': result = (::parse(b + 2, e, patient_info_) <= e); break;
      case 'L': return false;
    }
  }

  if (!result)
    throw std::runtime_error(contourpp::interface::to_string(b, e, "Can't parse record: "));

  return false;
}

void contourpp::record_parser::get_all(std::istream& is, std::vector<record>& records)
{
  records.clear();

  std::string buf;
  record rec;

  while (std::getline(is, buf))
  {
    if (parse(buf.data(), buf.data() + buf.size(), rec))
      records.push_back(rec);
  }
}


void contourpp::record_parser::get_all(std::vector<record>& records)
{
  records.clear();

  interface device;
  record rec;
  const char *begin = NULL, *end = NULL;

  while (device.sync(begin, end))
    if (parse(begin, end, rec))
      records.push_back(rec);
}
