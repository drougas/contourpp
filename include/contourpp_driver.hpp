#ifndef CONTOURPP_DRIVER_H__
#define CONTOURPP_DRIVER_H__

#include <istream>
#include <ostream>
#include <vector>
#include <boost/date_time.hpp>
//#include <boost/locale/date_time.hpp>
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
  unsigned char tags_, tag2_;

  const char* get_bayer_type() const;
  const char* get_bayer_type2() const;

public:
  record() : index_(0), value_(0), tags_(0), tag2_(0) {}

  record(const record& o)
    : datetime_(o.datetime_), index_(o.index_), value_(o.value_),
    tags_(o.tags_), tag2_(o.tag2_) {}

  record(const char* b, const char* e, char field_sep = '|')
  { parse_bayer(b, e, field_sep); }

  const datetime_t& datetime() const { return datetime_; }
  size_t index() const { return index_; }
  unsigned short value() const { return value_; }
  unsigned char min_after_meal() const { return is_glucose()? tag2_ : 0; }
  float hr_after_meal() const { return float(min_after_meal()) / 60; }

  bool      is_control() const { return (tags_ &  1) != 0; } // control
  bool  is_before_food() const { return (tags_ &  2) != 0; } // before food
  bool   is_after_food() const { return (tags_ &  4) != 0; } // after food
  bool dont_feel_right() const { return (tags_ &  8) != 0; } // don't feel right
  bool         is_sick() const { return (tags_ & 16) != 0; } // sick
  bool      has_stress() const { return (tags_ & 32) != 0; } // stress
  bool    has_activity() const { return (tags_ & 64) != 0; } // activity

  bool       is_glucose() const { return (tags_ & 128) == 0; }
  bool is_insulin_short() const { return (tags_ & 128) && (tag2_ == 0); }
  bool  is_insulin_long() const { return (tags_ & 128) && (tag2_ == 1); }
  bool         is_carbs() const { return (tags_ & 128) && (tag2_ == 2); }

  void clear() {
    datetime_ = datetime_t();
    index_ = 0;
    value_ = 0;
    tags_ = 0;
    tag2_ = 0;
  }

  void shift_time(const boost::posix_time::time_duration& d) { datetime_ += d; }

  bool parse_bayer(const char* b, const char* e, char field_sep = '|');

  template <typename _Elem, typename _Traits>
  void print_bayer(std::basic_ostream<_Elem,_Traits>& s, char field_sep = '|') const
  {
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y%m%d%H%M");
    s.imbue(std::locale(s.getloc(), facet));

    s << 'R' << field_sep << index_ << field_sep << get_bayer_type()
      << field_sep << value_ << field_sep << get_bayer_type2()
      << field_sep << field_sep;

    if (is_glucose()) {
      static const char letters[] = "CBADISX";
      bool attr_printed = false;
      for (size_t i = 0; i < 7; i++) {
        if (tags_ & (1 << i)) {
          if (attr_printed) s << '/';
          s << letters[i];
          attr_printed = true;
        }
      }

      if (tag2_ > 0) {
        char c = static_cast<char>(static_cast<unsigned short>(tag2_) / 15);
        c += (c > 9)? ('A' - 10) : '0';
        if (attr_printed) s << '/';
        s << 'Z' << c;
      }
    }

    s << field_sep << field_sep << datetime_;
  }

  bool parse_csv(const char* b, const char* e, char field_sep = ',');

  template <typename _Elem, typename _Traits>
  void print_csv(std::basic_ostream<_Elem,_Traits>& s, char field_sep = ',') const
  {
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M");
    s.imbue(std::locale(s.getloc(), facet));

    s << datetime_ << field_sep << value_;

    if (is_glucose()) {
      s << field_sep << (is_before_food()? "1" : (is_after_food()? "2" : ""))
        << field_sep << (dont_feel_right()? "1" : "")
        << field_sep << (is_sick()? "1" : "")
        << field_sep << (has_stress()? "1" : "")
        << field_sep << (has_activity()? "1" : "");

      if (tag2_)
        s << field_sep << hr_after_meal();
    }
    else
      s << field_sep << (-1 - static_cast<int>(tag2_));
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

  bool parse_H(const char* b, const char* e);
  bool parse_P(const char* b, const char* e);
  bool parse_O(const char*  , const char*  ) { throw std::exception(); }
  bool parse_R(const char* b, const char* e, record& rec) const {
    return rec.parse_bayer(b, e, field_sep_);
  }

public:
  record_parser()
    : field_sep_('|'), repeat_sep_('\\'), comp_sep_('^'),
    escape_sep_('&'), result_count_(0) {}

  bool parse(const char* b, const char* e, record& rec);
  void get_all(std::istream& is, std::vector<record>& records);
  void get_all(std::vector<record>& records);
};

} // namespace contourpp

#endif // CONTOURPP_DRIVER_H__
