#ifndef CONTOURPP_DRIVER_H__
#define CONTOURPP_DRIVER_H__

#include <boost/date_time.hpp>
//#include <boost/locale/date_time.hpp>
#include <istream>
#include <ostream>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time/posix_time/posix_time_io.hpp>

namespace contourpp
{

class record
{
public:
  //typedef boost::locale::date_time datetime_t;
  typedef boost::posix_time::ptime datetime_t;

private:
  datetime_t datetime_;
  size_t index_;
  unsigned short value_;
  unsigned char tags_, min_after_meal_;

public:
  record() : index_(0), value_(0), tags_(0), min_after_meal_(0) {}

  record(const record& o)
    : datetime_(o.datetime_), index_(o.index_), value_(o.value_),
    tags_(o.tags_), min_after_meal_(o.min_after_meal_) {}

  record(const char* const& b, const char* const& e, const char& field_sep = '|')
  { parse_bayer(b, e, field_sep); }

  const datetime_t& datetime() const { return datetime_; }
  const size_t& index() const { return index_; }
  const unsigned short& value() const { return value_; }
  const unsigned char& min_after_meal() const { return min_after_meal_; }
  float hr_after_meal() const { return float(min_after_meal_) / 60; }

  bool      is_control() const { return (tags_ &  1) != 0; } // control
  bool  is_before_food() const { return (tags_ &  2) != 0; } // before food
  bool   is_after_food() const { return (tags_ &  4) != 0; } // after food
  bool dont_feel_right() const { return (tags_ &  8) != 0; } // don't feel right
  bool         is_sick() const { return (tags_ & 16) != 0; } // sick
  bool      has_stress() const { return (tags_ & 32) != 0; } // stress
  bool    has_activity() const { return (tags_ & 64) != 0; } // activity

  void clear() {
    datetime_ = datetime_t();
    index_ = 0;
    value_ = 0;
    tags_ = 0;
    min_after_meal_ = 0;
  }

  void shift_time(const boost::posix_time::time_duration& d) { datetime_ += d; }

  bool parse_bayer(const char* b, const char* const& e, const char& field_sep = '|');

  template <typename _Elem, typename _Traits>
  void print_bayer(std::basic_ostream<_Elem,_Traits>& s, const char& field_sep = '|') const
  {
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y%m%d%H%M");
    s.imbue(std::locale(s.getloc(), facet));

    s << 'R' << field_sep << index_ << field_sep << "^^^Glucose" << field_sep
      << value_ << field_sep << "mg/dL^P" << field_sep << field_sep;

    static const char letters[] = "CBADISX";
    bool attr_printed = false;
    for (size_t i = 0; i < 7; i++) {
      if (tags_ & (1 << i)) {
        if (attr_printed) s << '/';
        s << letters[i];
        attr_printed = true;
      }
    }

    if (min_after_meal_ > 0) {
      char c = static_cast<char>(static_cast<unsigned short>(min_after_meal_) / 15);
      c += (c > 9)? ('A' - 10) : '0';
      if (attr_printed) s << '/';
      s << 'Z' << c;
    }

    s << field_sep << field_sep << datetime_;
  }

  template <typename _Elem, typename _Traits>
  void print_csv(std::basic_ostream<_Elem,_Traits>& s, const char& field_sep = ',') const
  {
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M");
    s.imbue(std::locale(s.getloc(), facet));

    s << datetime_ << field_sep << value_
      << field_sep << (is_before_food()? "1" : (is_after_food()? "2" : ""))
      << field_sep << (dont_feel_right()? "1" : "")
      << field_sep << (is_sick()? "1" : "")
      << field_sep << (has_stress()? "1" : "")
      << field_sep << (has_activity()? "1" : "");

    if (min_after_meal_)
      s << field_sep << hr_after_meal();
  }

  template <typename _Elem, typename _Traits>
  friend std::basic_ostream<_Elem,_Traits> & operator << (
    std::basic_ostream<_Elem,_Traits>& s, const record& r)
  {
    r.print_csv(s);
    return s;
  }
}; // class record


class record_parser
{
private:
  char field_sep_;
  char repeat_sep_;
  char comp_sep_;
  char escape_sep_;
  std::string password_;
  std::string product_, versions_, serial_, sku_, device_info_, patient_info_;
  size_t result_count_;

  bool parse_H(const char*        b, const char* const& e);
  bool parse_P(const char* const& b, const char* const& e);
  bool parse_O(const char* const&  , const char* const&  ) { throw std::exception(); }
  bool parse_R(const char* const& b, const char* const& e, record& rec) const {
    return rec.parse_bayer(b, e, field_sep_);
  }

public:
  record_parser()
    : field_sep_('|'), repeat_sep_('\\'), comp_sep_('^'),
    escape_sep_('&'), result_count_(0) {}

  bool parse(const char* const& b, const char* const& e, record& rec);
  void get_all(std::istream& is, std::vector<record>& records);
  void get_all(std::vector<record>& records);
};

} // namespace contourpp

#endif // CONTOURPP_DRIVER_H__
