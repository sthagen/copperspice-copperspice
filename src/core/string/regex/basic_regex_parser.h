/***********************************************************************
*
* Copyright (c) 2017-2025 Barbara Geller
* Copyright (c) 2017-2025 Ansel Sermersheim
*
* Copyright (c) 1998-2009 John Maddock
*
* This file is part of CopperSpice.
*
* CopperSpice is free software, released under the BSD 2-Clause license.
* For license details refer to LICENSE provided with this project.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://opensource.org/licenses/BSD-2-Clause
*
***********************************************************************/

/*
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef CS_BASIC_REGEX_PARSER_H
#define CS_BASIC_REGEX_PARSER_H

#include <cassert>
#include <limits>

namespace cs_regex_ns {

namespace cs_regex_detail_ns {

inline intmax_t umax()
{
   // extra parentheses around min, avoids expanding if it is a macro (MSVC issue)
   return (std::numeric_limits<intmax_t>::max)();
}

template <class charT, class traits>
class basic_regex_parser : public basic_regex_creator<charT, traits>
{
 public:
   basic_regex_parser(regex_data<charT, traits> *data);

   void parse(const typename traits::string_type::const_iterator iter_first,
              const typename traits::string_type::const_iterator iter_last, unsigned flags);

   void fail(regex_constants::error_type error_code, std::ptrdiff_t position);
   void fail(regex_constants::error_type error_code, std::ptrdiff_t position, std::string message, std::ptrdiff_t start_pos);
   void fail(regex_constants::error_type error_code, std::ptrdiff_t position, const std::string &message) {
      fail(error_code, position, message, position);
   }

   bool parse_all();
   bool parse_basic();
   bool parse_extended();
   bool parse_literal();
   bool parse_open_paren();
   bool parse_basic_escape();
   bool parse_extended_escape();
   bool parse_match_any();
   bool parse_repeat(std::size_t low = 0, std::size_t high = (std::numeric_limits<std::size_t>::max)());
   bool parse_repeat_range(bool isbasic);
   bool parse_alt();
   bool parse_set();
   bool parse_backref();
   void parse_set_literal(basic_char_set<charT, traits> &char_set);
   bool parse_inner_set(basic_char_set<charT, traits> &char_set);
   bool parse_QE();
   bool parse_perl_extension();
   bool parse_perl_verb();
   bool match_verb(const char *);
   bool add_emacs_code(bool negate);

   bool unwind_alts(std::ptrdiff_t last_paren_start);

   digraph<charT> get_next_set_literal(basic_char_set<charT, traits> &char_set);
   charT unescape_character();
   regex_constants::syntax_option_type parse_options();

 private:
   using parser_proc_type = bool (basic_regex_parser::*)();
   using string_type      = typename traits::string_type;
   using char_class_type  = typename traits::char_class_type;

   parser_proc_type           m_parser_proc;                       // the main parser to use

   typename traits::string_type::const_iterator  m_base;           // the start of the string being parsed
   typename traits::string_type::const_iterator  m_end;            // the end of the string being parsed
   typename traits::string_type::const_iterator  m_position;       // our current parser position

   unsigned                   m_mark_count;        // how many sub-expressions we have
   int                        m_mark_reset;        // used to indicate that we're inside a (?|...) block.
   unsigned                   m_max_mark;          // largest mark count seen inside a (?|...) block.
   std::ptrdiff_t             m_paren_start;       // where the last seen ')' began (where repeats are inserted).
   std::ptrdiff_t             m_alt_insert_point;  // where to insert the next alternative
   bool                       m_has_case_change;   // true if somewhere in the current block the case has changed
   unsigned                   m_recursion_count;   // How many times we've called parse_all.

   std::vector<std::ptrdiff_t> m_alt_jumps;        // list of alternative in the current scope.

   basic_regex_parser &operator=(const basic_regex_parser &);
   basic_regex_parser(const basic_regex_parser &);
};

template <class charT, class traits>
basic_regex_parser<charT, traits>::basic_regex_parser(regex_data<charT, traits> *data)
   : basic_regex_creator<charT, traits>(data), m_mark_count(0), m_mark_reset(-1), m_max_mark(0), m_paren_start(0),
     m_alt_insert_point(0), m_has_case_change(false), m_recursion_count(0)
{
}

template <class charT, class traits>
void basic_regex_parser<charT, traits>::parse(const typename traits::string_type::const_iterator iter_first,
      const typename traits::string_type::const_iterator iter_last, unsigned l_flags)
{
   // pass l_flags on to base class
   this->init(l_flags);

   // set up pointers
   m_position = iter_first;
   m_base     = iter_first;
   m_end      = iter_last;

   // empty strings are errors
   if ((iter_first == iter_last) && ( ((l_flags & regbase::main_option_type) != regbase::perl_syntax_group) ||
                  (l_flags & regbase::no_empty_expressions)) ) {
      fail(regex_constants::error_empty, 0);
      return;
   }

   // select which parser to use
   switch (l_flags & regbase::main_option_type) {

      case regbase::perl_syntax_group: {
         m_parser_proc = &basic_regex_parser<charT, traits>::parse_extended;

         // Add a leading paren with index zero to give recursions a target

         re_brace *br = static_cast<re_brace *>(this->append_state(syntax_element_startmark, sizeof(re_brace)));
         br->index    = 0;
         br->icase    = this->flags() & regbase::icase;

         break;
      }

      case regbase::basic_syntax_group:
         m_parser_proc = &basic_regex_parser<charT, traits>::parse_basic;
         break;

      case regbase::literal:
         m_parser_proc = &basic_regex_parser<charT, traits>::parse_literal;
         break;

      default:
         // someone has managed to set more than one of the main option flags
         fail(regex_constants::error_unknown, 0, "An invalid combination of regular expression syntax flags was used.");
         return;
   }

   // parse all our characters
   bool result = parse_all();

   // Unwind our alternatives:
   unwind_alts(-1);

   // reset l_flags as a global scope (?imsx) may have altered them
   this->flags(l_flags);

   // if we haven't used up all the characters then we must
   // have had an unexpected ')' :

   if (! result) {
      fail(regex_constants::error_paren, std::distance(m_base, m_position),
           "Found a closing ) with no corresponding openening parenthesis.");
      return;
   }

   // if an error has been set then give up now
   if (this->m_pdata->m_status) {
      return;
   }

   // fill in our sub-expression count:
   this->m_pdata->m_mark_count = 1 + m_mark_count;
   this->finalize(iter_first, iter_last);
}

template <class charT, class traits>
void basic_regex_parser<charT, traits>::fail(regex_constants::error_type error_code, std::ptrdiff_t position)
{
   // get the error message:
   std::string message = this->m_pdata->m_ptraits->error_string(error_code);
   fail(error_code, position, message);
}

template <class charT, class traits>
void basic_regex_parser<charT, traits>::fail(regex_constants::error_type error_code, std::ptrdiff_t position,
                  std::string message, std::ptrdiff_t start_pos)
{
   if (0 == this->m_pdata->m_status) {
      // update the error code if not already set
      this->m_pdata->m_status = error_code;
   }

   m_position = m_end; // do not bother parsing anything else

   // Augment error message with the regular expression text
   if (start_pos == position) {
      start_pos = (std::max)(static_cast<std::ptrdiff_t>(0), position - static_cast<std::ptrdiff_t>(10));
   }

   std::ptrdiff_t end_pos = (std::min)(position + static_cast<std::ptrdiff_t>(10),
                  static_cast<std::ptrdiff_t>(m_end - m_base));

   if (error_code != regex_constants::error_empty) {

      if ((start_pos != 0) || (end_pos != (m_end - m_base))) {
         message.append("  Error occurred while parsing the regular expression fragment: '");
      } else {
         message.append("  Error occurred while parsing the regular expression: '");
      }

      if (start_pos != end_pos) {
         typename traits::string_type text{m_base + position, m_base + end_pos};

         auto tmp = text.toUtf8();
         message.append(tmp.begin(), tmp.end());
      }

      message.append("'.");
   }

   if (0 == (this->flags() & regex_constants::no_except)) {
      cs_regex_ns::regex_error e(message, error_code, position);
      e.raise();
   }
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_all()
{

   if (++m_recursion_count > 400) {
      // exceeded internal limits
      fail(cs_regex_ns::regex_constants::error_complexity, m_position - m_base, "Exceeded nested brace limit.");
   }

   bool result = true;

   while (result && (m_position != m_end)) {
      result = (this->*m_parser_proc)();
   }

   --m_recursion_count;

   return result;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_basic()
{
   switch (this->m_traits.syntax_type(*m_position)) {
      case regex_constants::syntax_escape:
         return parse_basic_escape();

      case regex_constants::syntax_dot:
         return parse_match_any();

      case regex_constants::syntax_caret:
         ++m_position;
         this->append_state(syntax_element_start_line);
         break;

      case regex_constants::syntax_dollar:
         ++m_position;
         this->append_state(syntax_element_end_line);
         break;

      case regex_constants::syntax_star:
         if (! (this->m_last_state) || (this->m_last_state->type == syntax_element_start_line)) {
            return parse_literal();
         } else {
            ++m_position;
            return parse_repeat();
         }

      case regex_constants::syntax_plus:
         if (!(this->m_last_state) || (this->m_last_state->type == syntax_element_start_line) || !(this->flags() & regbase::emacs_ex)) {
            return parse_literal();
         } else {
            ++m_position;
            return parse_repeat(1);
         }

      case regex_constants::syntax_question:
         if (!(this->m_last_state) || (this->m_last_state->type == syntax_element_start_line) || !(this->flags() & regbase::emacs_ex)) {
            return parse_literal();
         } else {
            ++m_position;
            return parse_repeat(0, 1);
         }

      case regex_constants::syntax_open_set:
         return parse_set();

      case regex_constants::syntax_newline:
         if (this->flags() & regbase::newline_alt) {
            return parse_alt();
         } else {
            return parse_literal();
         }

      default:
         return parse_literal();
   }

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_extended()
{
   bool result = true;

   switch (this->m_traits.syntax_type(*m_position)) {

      case regex_constants::syntax_open_mark:
         return parse_open_paren();

      case regex_constants::syntax_close_mark:
         return false;

      case regex_constants::syntax_escape:
         return parse_extended_escape();

      case regex_constants::syntax_dot:
         return parse_match_any();

      case regex_constants::syntax_caret:
         ++m_position;
         this->append_state(
            (this->flags() & regex_constants::no_mod_m ? syntax_element_buffer_start : syntax_element_start_line));
         break;

      case regex_constants::syntax_dollar:
         ++m_position;
         this->append_state(
            (this->flags() & regex_constants::no_mod_m ? syntax_element_buffer_end : syntax_element_end_line));
         break;

      case regex_constants::syntax_star:
         if (m_position == this->m_base) {
            fail(regex_constants::error_badrepeat, 0, "The repeat operator \"*\" cannot start a regular expression.");
            return false;
         }
         ++m_position;
         return parse_repeat();

      case regex_constants::syntax_question:
         if (m_position == this->m_base) {
            fail(regex_constants::error_badrepeat, 0, "The repeat operator \"?\" cannot start a regular expression.");
            return false;
         }
         ++m_position;
         return parse_repeat(0, 1);

      case regex_constants::syntax_plus:
         if (m_position == this->m_base) {
            fail(regex_constants::error_badrepeat, 0, "The repeat operator \"+\" cannot start a regular expression.");
            return false;
         }
         ++m_position;
         return parse_repeat(1);

      case regex_constants::syntax_open_brace:
         ++m_position;
         return parse_repeat_range(false);

      case regex_constants::syntax_close_brace:
         if ((this->flags() & regbase::no_perl_ex) == regbase::no_perl_ex) {
            fail(regex_constants::error_brace, this->m_position - this->m_base,
                  "Found a closing repetition operator } with no corresponding {.");
            return false;
         }

         result = parse_literal();
         break;

      case regex_constants::syntax_or:
         return parse_alt();

      case regex_constants::syntax_open_set:
         return parse_set();

      case regex_constants::syntax_newline:
         if (this->flags() & regbase::newline_alt) {
            return parse_alt();
         } else {
            return parse_literal();
         }

      case regex_constants::syntax_hash:

         // If we have a mod_x flag set, then skip until
         // we get to a newline character:

         if ((this->flags() & (regbase::no_perl_ex | regbase::mod_x)) == regbase::mod_x) {
            while ((m_position != m_end) && ! is_separator(*m_position++)) {
            }

            return true;
         }
         [[fallthrough]];

      default:
         result = parse_literal();
         break;
   }
   return result;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_literal()
{
   // append this as a literal provided it is not a space character
   // or the perl option regbase::mod_x is not set

   if (((this->flags() & (regbase::main_option_type | regbase::mod_x | regbase::no_perl_ex)) != regbase::mod_x) ||
         ! this->m_traits.isctype(*m_position, this->m_mask_space)) {

      this->append_literal(*m_position);
   }

   ++m_position;

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_open_paren()
{
   // skip the '(' and error check

   if (++m_position == m_end) {
      fail(regex_constants::error_paren, m_position - m_base);
      return false;
   }

   // begin by checking for a perl-style (?...) extension
   if (((this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) == 0) ||
            ((this->flags() & (regbase::main_option_type | regbase::emacs_ex)) == (regbase::basic_syntax_group | regbase::emacs_ex))) {

      if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_question) {
         return parse_perl_extension();
      }

      if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_star) {
         return parse_perl_verb();
      }
   }

   // update our mark count, and append the required state:
   unsigned markid = 0;

   if (0 == (this->flags() & regbase::nosubs)) {
      markid = ++m_mark_count;

      if (this->flags() & regbase::save_subexpression_location) {
         this->m_pdata->m_subs.push_back(std::pair<std::size_t, std::size_t>(std::distance(m_base, m_position) - 1, 0));
      }
   }

   re_brace *pb = static_cast<re_brace *>(this->append_state(syntax_element_startmark, sizeof(re_brace)));
   pb->index = markid;
   pb->icase = this->flags() & regbase::icase;
   std::ptrdiff_t last_paren_start = this->getoffset(pb);

   // back up insertion point for alternations, and set new point:
   std::ptrdiff_t last_alt_point = m_alt_insert_point;
   this->m_pdata->m_data.align();
   m_alt_insert_point = this->m_pdata->m_data.size();


   // back up the current flags in case we have a nested (?imsx) group
   regex_constants::syntax_option_type opts = this->flags();
   bool old_case_change = m_has_case_change;
   m_has_case_change = false; // no changes to this scope as yet...


   // Back up branch reset data in case we have a nested (?|...)
   int mark_reset = m_mark_reset;
   m_mark_reset = -1;

   // now recursively add more states, this will terminate when we get to a matching ')' :
   parse_all();

   // Unwind pushed alternatives
   if (0 == unwind_alts(last_paren_start)) {
      return false;
   }

   // restore flags
   if (m_has_case_change) {
      // the case has changed in one or more of the alternatives
      // within the scoped (...) block: we have to add a state
      // to reset the case sensitivity:
      static_cast<re_case *>(
         this->append_state(syntax_element_toggle_case, sizeof(re_case))
      )->icase = opts & regbase::icase;
   }

   this->flags(opts);
   m_has_case_change = old_case_change;

   // restore branch reset:
   m_mark_reset = mark_reset;

   // we either have a ')' or we have run out of characters prematurely:
   if (m_position == m_end) {
      this->fail(regex_constants::error_paren, std::distance(m_base, m_end));
      return false;
   }

   if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
      return false;
   }

   if (markid && (this->flags() & regbase::save_subexpression_location)) {
      this->m_pdata->m_subs.at(markid - 1).second = std::distance(m_base, m_position);
   }

   ++m_position;

   // append closing parenthesis state:
   pb = static_cast<re_brace *>(this->append_state(syntax_element_endmark, sizeof(re_brace)));
   pb->index = markid;
   pb->icase = this->flags() & regbase::icase;

   this->m_paren_start = last_paren_start;

   // restore the alternate insertion point:

   this->m_alt_insert_point = last_alt_point;

   // allow backrefs to this mark:
   if ((markid > 0) && (markid < sizeof(unsigned) * CHAR_BIT)) {
      this->m_backrefs |= 1u << (markid - 1);
   }

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_basic_escape()
{
   if (++m_position == m_end) {
      fail(regex_constants::error_paren, m_position - m_base);
      return false;
   }

   bool result = true;

   switch (this->m_traits.escape_syntax_type(*m_position)) {
      case regex_constants::syntax_open_mark:
         return parse_open_paren();

      case regex_constants::syntax_close_mark:
         return false;

      case regex_constants::syntax_plus:
         if (this->flags() & regex_constants::bk_plus_qm) {
            ++m_position;
            return parse_repeat(1);
         } else {
            return parse_literal();
         }

      case regex_constants::syntax_question:
         if (this->flags() & regex_constants::bk_plus_qm) {
            ++m_position;
            return parse_repeat(0, 1);
         } else {
            return parse_literal();
         }

      case regex_constants::syntax_open_brace:
         if (this->flags() & regbase::no_intervals) {
            return parse_literal();
         }

         ++m_position;
         return parse_repeat_range(true);

      case regex_constants::syntax_close_brace:
         if (this->flags() & regbase::no_intervals) {
            return parse_literal();
         }

         fail(regex_constants::error_brace, this->m_position - this->m_base,
                  "Found a closing repetition operator } with no corresponding {.");
         return false;

      case regex_constants::syntax_or:
         if (this->flags() & regbase::bk_vbar) {
            return parse_alt();
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::syntax_digit:
         return parse_backref();

      case regex_constants::escape_type_start_buffer:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_buffer_start);
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::escape_type_end_buffer:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_buffer_end);
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::escape_type_word_assert:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_word_boundary);
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::escape_type_not_word_assert:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_within_word);
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::escape_type_left_word:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_word_start);
         } else {
            result = parse_literal();
         }
         break;

      case regex_constants::escape_type_right_word:
         if (this->flags() & regbase::emacs_ex) {
            ++m_position;
            this->append_state(syntax_element_word_end);
         } else {
            result = parse_literal();
         }
         break;

      default:
         if (this->flags() & regbase::emacs_ex) {
            bool negate = true;

            const auto tmp = *m_position;

            if (tmp == 'w') {
               negate = false;
            }

            if (tmp == 'w' || tmp == 'W') {
               basic_char_set<charT, traits> char_set;

               if (negate) {
                  char_set.negate();
               }

               char_set.add_class(this->m_word_mask);

               if (this->append_set(char_set) == nullptr) {
                  fail(regex_constants::error_ctype, m_position - m_base);
                  return false;
               }

               ++m_position;
               return true;

            } else if (tmp == 's') {
               negate = false;
               return add_emacs_code(negate);

            } else if (tmp == 'S') {
               return add_emacs_code(negate);

            } else if (tmp == 'c' || tmp == 'C') {
               // not supported yet

               fail(regex_constants::error_escape, m_position - m_base,
                    "The \\c and \\C escape sequences are not supported by POSIX basic regular expressions: try the Perl syntax instead.");
               return false;
            }
         }

         result = parse_literal();
         break;
   }

   return result;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_extended_escape()
{
   ++m_position;

   if (m_position == m_end) {
      fail(regex_constants::error_escape, m_position - m_base, "Incomplete escape sequence found.");
      return false;
   }

   bool negate = false;    // in case this is a character class escape: \w \d etc

   switch (this->m_traits.escape_syntax_type(*m_position)) {

      case regex_constants::escape_type_not_class:
         negate = true;
         [[fallthrough]];

      case regex_constants::escape_type_class: {

         escape_type_class_jump:
            using m_type = typename traits::char_class_type;

            m_type m = this->m_traits.lookup_classname(m_position, m_position + 1);

            if (m != 0) {
               basic_char_set<charT, traits> char_set;

               if (negate) {
                  char_set.negate();
               }

               char_set.add_class(m);

               if (this->append_set(char_set) == nullptr) {
                  fail(regex_constants::error_ctype, m_position - m_base);
                  return false;
               }
               ++m_position;
               return true;
            }

            // not a class, just a regular unknown escape:
            this->append_literal(unescape_character());
            break;
         }

      case regex_constants::syntax_digit:
         return parse_backref();

      case regex_constants::escape_type_left_word:
         ++m_position;
         this->append_state(syntax_element_word_start);
         break;

      case regex_constants::escape_type_right_word:
         ++m_position;
         this->append_state(syntax_element_word_end);
         break;

      case regex_constants::escape_type_start_buffer:
         ++m_position;
         this->append_state(syntax_element_buffer_start);
         break;

      case regex_constants::escape_type_end_buffer:
         ++m_position;
         this->append_state(syntax_element_buffer_end);
         break;

      case regex_constants::escape_type_word_assert:
         ++m_position;
         this->append_state(syntax_element_word_boundary);
         break;

      case regex_constants::escape_type_not_word_assert:
         ++m_position;
         this->append_state(syntax_element_within_word);
         break;

      case regex_constants::escape_type_Z:
         ++m_position;
         this->append_state(syntax_element_soft_buffer_end);
         break;

      case regex_constants::escape_type_Q:
         return parse_QE();

      case regex_constants::escape_type_C:
         return parse_match_any();

      case regex_constants::escape_type_X:
         ++m_position;
         this->append_state(syntax_element_combining);
         break;

      case regex_constants::escape_type_G:
         ++m_position;
         this->append_state(syntax_element_restart_continue);
         break;

      case regex_constants::escape_type_not_property:
         negate = true;
         [[fallthrough]];

      case regex_constants::escape_type_property: {
         ++m_position;
         char_class_type m;

         if ( m_position == m_end) {
            fail(regex_constants::error_escape, m_position - m_base, "Incomplete property escape found.");
            return false;
         }

         // maybe have \p{ddd}
         if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_open_brace) {
            typename traits::string_type::const_iterator base = m_position;

            // skip forward until we find enclosing brace:
            while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_brace)) {
               ++m_position;
            }

            if (m_position == m_end) {
               fail(regex_constants::error_escape, m_position - m_base, "Closing } missing from property escape sequence.");
               return false;
            }

            m = this->m_traits.lookup_classname(++base, m_position++);

         } else {
            m = this->m_traits.lookup_classname(m_position, m_position + 1);
            ++m_position;
         }

         if (m != 0) {
            basic_char_set<charT, traits> char_set;
            if (negate) {
               char_set.negate();
            }

            char_set.add_class(m);

            if (this->append_set(char_set) == nullptr) {
               fail(regex_constants::error_ctype, m_position - m_base);
               return false;
            }

            return true;
         }

         fail(regex_constants::error_ctype, m_position - m_base,
              "Escape sequence was neither a valid property nor a valid character class name.");
         return false;
      }

      case regex_constants::escape_type_reset_start_mark:
         if (0 == (this->flags() & (regbase::main_option_type | regbase::no_perl_ex))) {
            re_brace *pb = static_cast<re_brace *>(this->append_state(syntax_element_startmark, sizeof(re_brace)));
            pb->index    = -5;
            pb->icase    = this->flags() & regbase::icase;

            this->m_pdata->m_data.align();
            ++m_position;
            return true;
         }

         goto escape_type_class_jump;

      case regex_constants::escape_type_line_ending:
         if (0 == (this->flags() & (regbase::main_option_type | regbase::no_perl_ex))) {
            const typename traits::string_type &tmpRegexp = get_escape_R_string<typename traits::string_type>();

            typename traits::string_type::const_iterator old_position = m_position;
            typename traits::string_type::const_iterator old_end      = m_end;
            typename traits::string_type::const_iterator old_base     = m_base;

            m_position = tmpRegexp.begin();
            m_base     = tmpRegexp.begin();
            m_end      = tmpRegexp.end();

            bool r     = parse_all();

            m_position = ++old_position;
            m_end      = old_end;
            m_base     = old_base;

            return r;
         }

         goto escape_type_class_jump;

      case regex_constants::escape_type_extended_backref:
         if (0 == (this->flags() & (regbase::main_option_type | regbase::no_perl_ex))) {
            bool have_brace = false;
            bool negative   = false;

            static const char *incomplete_message = "Incomplete \\g escape found.";

            if (++m_position == m_end) {
               fail(regex_constants::error_escape, m_position - m_base, incomplete_message);
               return false;
            }
            // maybe have \g{ddd}
            regex_constants::syntax_type syn = this->m_traits.syntax_type(*m_position);
            regex_constants::syntax_type syn_end = 0;

            if ((syn == regex_constants::syntax_open_brace)
                  || (syn == regex_constants::escape_type_left_word)
                  || (syn == regex_constants::escape_type_end_buffer)) {
               if (++m_position == m_end) {
                  fail(regex_constants::error_escape, m_position - m_base, incomplete_message);
                  return false;
               }
               have_brace = true;
               switch (syn) {
                  case regex_constants::syntax_open_brace:
                     syn_end = regex_constants::syntax_close_brace;
                     break;
                  case regex_constants::escape_type_left_word:
                     syn_end = regex_constants::escape_type_right_word;
                     break;
                  default:
                     syn_end = regex_constants::escape_type_end_buffer;
                     break;
               }
            }

            negative = (*m_position == static_cast<charT>('-'));

            if ((negative) && (++m_position == m_end)) {
               fail(regex_constants::error_escape, m_position - m_base, incomplete_message);
               return false;
            }

            typename traits::string_type::const_iterator pc = m_position;
            intmax_t i = this->m_traits.toi(pc, m_end, 10);

            if ((i < 0) && syn_end) {
               // Check for a named capture, get the leftmost one if there is more than one:
               const typename traits::string_type::const_iterator base = m_position;

               while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != syn_end)) {
                  ++m_position;
               }

               i = hash_value_from_capture_name(base, m_position);
               pc = m_position;
            }

            if (negative) {
               i = 1 + m_mark_count - i;
            }

            if (((i > 0) && (i < std::numeric_limits<unsigned>::digits) && (i - 1 < static_cast<intmax_t>(sizeof(unsigned) * CHAR_BIT)) &&
                  (this->m_backrefs & (1u << (i - 1)))) || ((i > 10000) && (this->m_pdata->get_id(i) > 0) &&
                        (this->m_pdata->get_id(i) - 1 < static_cast<intmax_t>(sizeof(unsigned) * CHAR_BIT)) && (this->m_backrefs &
                        (1u << (this->m_pdata->get_id(i) - 1))))) {

               m_position = pc;
               re_brace *pb = static_cast<re_brace *>(this->append_state(syntax_element_backref, sizeof(re_brace)));
               pb->index = i;
               pb->icase = this->flags() & regbase::icase;

            } else {
               fail(regex_constants::error_backref, m_position - m_base);
               return false;
            }

            m_position = pc;

            if (have_brace) {
               if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != syn_end)) {
                  fail(regex_constants::error_escape, m_position - m_base, incomplete_message);
                  return false;
               }
               ++m_position;
            }
            return true;
         }
         goto escape_type_class_jump;

      case regex_constants::escape_type_control_v:
         if (0 == (this->flags() & (regbase::main_option_type | regbase::no_perl_ex))) {
            goto escape_type_class_jump;
         }
         [[fallthrough]];

      default:
         this->append_literal(unescape_character());
         break;
   }

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_match_any()
{
   //
   // we have a '.' that can match any character:
   //
   ++m_position;
   static_cast<re_dot *>(
      this->append_state(syntax_element_wild, sizeof(re_dot))
   )->mask = static_cast<unsigned char>(this->flags() & regbase::no_mod_s
                                        ? cs_regex_detail_ns::force_not_newline
                                        : this->flags() & regbase::mod_s ?
                                        cs_regex_detail_ns::force_newline : cs_regex_detail_ns::dont_care);
   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_repeat(std::size_t low, std::size_t high)
{
   bool greedy = true;
   bool pocessive = false;
   std::size_t insert_point;

   // when we get to here we may have a non-greedy ? mark still to come:

   if ((m_position != m_end) && (
            (0 == (this->flags() & (regbase::main_option_type | regbase::no_perl_ex))) ||
            ((regbase::basic_syntax_group | regbase::emacs_ex) == (this->flags() & (regbase::main_option_type | regbase::emacs_ex))) )) {

      // have a perl or emacs regex, check for a '?':
      if ((this->flags() & (regbase::main_option_type | regbase::mod_x | regbase::no_perl_ex)) == regbase::mod_x) {
         // whitespace skip

         while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
            ++m_position;
         }
      }

      if ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_question)) {
         greedy = false;
         ++m_position;
      }

      // for perl regexes only check for pocessive ++ repeats.
      if ((m_position != m_end) && (0 == (this->flags() & regbase::main_option_type))
            && (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_plus)) {

         pocessive = true;
         ++m_position;
      }
   }

   if (this->m_last_state == nullptr) {
      fail(regex_constants::error_badrepeat, std::distance(m_base, m_position), "Nothing to repeat.");
      return false;
   }

   if (this->m_last_state->type == syntax_element_endmark) {
      // insert a repeat before the '(' matching the last ')':
      insert_point = this->m_paren_start;

   } else if ((this->m_last_state->type == syntax_element_literal) && (static_cast<re_literal *>(this->m_last_state)->length > 1)) {
      // the last state was a literal with more than one character, split it in two:
      re_literal *lit = static_cast<re_literal *>(this->m_last_state);
      charT c = (static_cast<charT *>(static_cast<void *>(lit + 1)))[lit->length - 1];
      lit->length -= 1;

      // now append new state:
      lit = static_cast<re_literal *>(this->append_state(syntax_element_literal, sizeof(re_literal) + sizeof(charT)));
      lit->length = 1;
      (static_cast<charT *>(static_cast<void *>(lit + 1)))[0] = c;
      insert_point = this->getoffset(this->m_last_state);

   } else {
      // repeat the last state whatever it was, need to add some error checking here:
      switch (this->m_last_state->type) {
         case syntax_element_start_line:
         case syntax_element_end_line:
         case syntax_element_word_boundary:
         case syntax_element_within_word:
         case syntax_element_word_start:
         case syntax_element_word_end:
         case syntax_element_buffer_start:
         case syntax_element_buffer_end:
         case syntax_element_alt:
         case syntax_element_soft_buffer_end:
         case syntax_element_restart_continue:
         case syntax_element_jump:
         case syntax_element_startmark:
         case syntax_element_backstep:
            // can't legally repeat any of the above:
            fail(regex_constants::error_badrepeat, m_position - m_base);
            return false;

         default:
            // do nothing...
            break;
      }

      insert_point = this->getoffset(this->m_last_state);
   }

   // OK we now know what to repeat, so insert the repeat around it
   re_repeat *rep = static_cast<re_repeat *>(this->insert_state(insert_point, syntax_element_rep, re_repeater_size));
   rep->min     = low;
   rep->max     = high;
   rep->greedy  = greedy;
   rep->leading = false;

   // store our repeater position for later:
   std::ptrdiff_t rep_off = this->getoffset(rep);
   // and append a back jump to the repeat:
   re_jump *jmp = static_cast<re_jump *>(this->append_state(syntax_element_jump, sizeof(re_jump)));
   jmp->alt.i = rep_off - this->getoffset(jmp);
   this->m_pdata->m_data.align();
   // now fill in the alt jump for the repeat:
   rep = static_cast<re_repeat *>(this->getaddress(rep_off));
   rep->alt.i = this->m_pdata->m_data.size() - rep_off;

   // If the repeat is pocessive then bracket the repeat with a (?>...)
   // independent sub-expression construct:

   if (pocessive) {
      if (m_position != m_end) {

         // Check for illegal following quantifier, we have to do this here, because
         // the extra states we insert below circumvents our usual error checking :-(

         bool contin = false;

         do {
            if ((this->flags() & (regbase::main_option_type | regbase::mod_x | regbase::no_perl_ex)) == regbase::mod_x) {

               // whitespace skip
               while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
                  ++m_position;
               }
            }
            if (m_position != m_end) {
               switch (this->m_traits.syntax_type(*m_position)) {
                  case regex_constants::syntax_star:
                  case regex_constants::syntax_plus:
                  case regex_constants::syntax_question:
                  case regex_constants::syntax_open_brace:
                     fail(regex_constants::error_badrepeat, m_position - m_base);
                     return false;
                  case regex_constants::syntax_open_mark:
                     // Do we have a comment?  If so we need to skip it here...
                     if ((m_position + 2 < m_end) && this->m_traits.syntax_type(*(m_position + 1)) == regex_constants::syntax_question
                           && this->m_traits.syntax_type(*(m_position + 2)) == regex_constants::syntax_hash) {
                        while ((m_position != m_end)
                               && (this->m_traits.syntax_type(*m_position++) != regex_constants::syntax_close_mark)) {
                        }
                        contin = true;
                     } else {
                        contin = false;
                     }
               }
            } else {
               contin = false;
            }
         } while (contin);
      }

      re_brace *pb = static_cast<re_brace *>(this->insert_state(insert_point, syntax_element_startmark, sizeof(re_brace)));
      pb->index = -3;
      pb->icase = this->flags() & regbase::icase;

      jmp = static_cast<re_jump *>(this->insert_state(insert_point + sizeof(re_brace), syntax_element_jump, sizeof(re_jump)));
      this->m_pdata->m_data.align();

      jmp->alt.i = this->m_pdata->m_data.size() - this->getoffset(jmp);
      pb = static_cast<re_brace *>(this->append_state(syntax_element_endmark, sizeof(re_brace)));
      pb->index = -3;
      pb->icase = this->flags() & regbase::icase;
   }

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_repeat_range(bool isbasic)
{
   static const char *incomplete_message = "Missing } in quantified repetition.";

   // parse a repeat-range
   std::size_t min;
   std::size_t max;
   intmax_t    v;

   // skip whitespace
   while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
      ++m_position;
   }

   if (this->m_position == this->m_end) {
      if (this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) {
         fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
         return false;
      }

      // Treat the opening '{' as a literal character, rewind to start of error:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
         --m_position;
      }

      return parse_literal();
   }

   // get min
   v = this->m_traits.toi(m_position, m_end, 10);

   // skip whitespace
   if ((v < 0) || (v > umax())) {
      if (this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) {
         fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
         return false;
      }

      // Treat the opening '{' as a literal character, rewind to start of error:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
         --m_position;
      }
      return parse_literal();
   }

   while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
      ++m_position;
   }

   if (this->m_position == this->m_end) {
      if (this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) {
         fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
         return false;
      }
      // Treat the opening '{' as a literal character, rewind to start of error:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
         --m_position;
      }
      return parse_literal();
   }

   min = static_cast<std::size_t>(v);

   // see if we have a comma:
   if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_comma) {
      // move on and error check:
      ++m_position;

      // skip whitespace:
      while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
         ++m_position;
      }

      if (this->m_position == this->m_end) {
         if (this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) {
            fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
            return false;
         }

         // Treat the opening '{' as a literal character, rewind to start of error:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
            --m_position;
         }

         return parse_literal();
      }
      // get the value if any:
      v = this->m_traits.toi(m_position, m_end, 10);
      max = ((v >= 0) && (v < umax())) ? (std::size_t)v : (std::numeric_limits<std::size_t>::max)();
   } else {
      // no comma, max = min:
      max = min;
   }

   // skip whitespace:
   while ((m_position != m_end) && this->m_traits.isctype(*m_position, this->m_mask_space)) {
      ++m_position;
   }

   // OK now check trailing }:
   if (this->m_position == this->m_end) {
      if (this->flags() & (regbase::main_option_type | regbase::no_perl_ex)) {
         fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
         return false;
      }
      // Treat the opening '{' as a literal character, rewind to start of error:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
         --m_position;
      }
      return parse_literal();
   }

   if (isbasic) {
      if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_escape) {
         ++m_position;
         if (this->m_position == this->m_end) {
            fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
            return false;
         }
      } else {
         fail(regex_constants::error_brace, this->m_position - this->m_base, incomplete_message);
         return false;
      }
   }
   if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_close_brace) {
      ++m_position;
   } else {
      // Treat the opening '{' as a literal character, rewind to start of error:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_brace) {
         --m_position;
      }

      return parse_literal();
   }

   // finally go and add the repeat, unless error:
   if (min > max) {
      // Backtrack to error location:
      m_position -= 2;
      while (this->m_traits.isctype(*m_position, this->m_word_mask)) {
         --m_position;
      }

      ++m_position;
      fail(regex_constants::error_badbrace, m_position - m_base);
      return false;
   }

   return parse_repeat(min, max);
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_alt()
{
   //
   // error check: if there have been no previous states,
   // or if the last state was a '(' then error:
   //
   if (
      ((this->m_last_state == nullptr) || (this->m_last_state->type == syntax_element_startmark)) &&
      !( ((this->flags() & regbase::main_option_type) == regbase::perl_syntax_group) &&
         ((this->flags() & regbase::no_empty_expressions) == 0) )) {
      fail(regex_constants::error_empty, this->m_position - this->m_base,
                  "A regular expression cannot start with the alternation operator |.");

      return false;
   }
   //
   // Reset mark count if required:
   //
   if (m_max_mark < m_mark_count) {
      m_max_mark = m_mark_count;
   }
   if (m_mark_reset >= 0) {
      m_mark_count = m_mark_reset;
   }

   ++m_position;

   // we need to append a trailing jump:
   re_syntax_base *pj = this->append_state(cs_regex_detail_ns::syntax_element_jump, sizeof(re_jump));
   std::ptrdiff_t jump_offset = this->getoffset(pj);

   //
   // now insert the alternative:
   //
   re_alt *palt = static_cast<re_alt *>(this->insert_state(this->m_alt_insert_point, syntax_element_alt, re_alt_size));
   jump_offset += re_alt_size;
   this->m_pdata->m_data.align();

   palt->alt.i = this->m_pdata->m_data.size() - this->getoffset(palt);

   //
   // update m_alt_insert_point so that the next alternate gets
   // inserted at the start of the second of the two we've just created:
   //
   this->m_alt_insert_point = this->m_pdata->m_data.size();

   //
   // the start of this alternative must have a case changes state
   // if the current block has messed around with case changes:
   //
   if (m_has_case_change) {
      static_cast<re_case *>(
         this->append_state(syntax_element_toggle_case, sizeof(re_case))
      )->icase = this->m_icase;
   }

   //
   // push the alternative onto our stack, a recursive
   // implementation here is easier to understand (and faster
   // as it happens), but causes all kinds of stack overflow problems
   // on programs with small stacks (COM+).
   //
   m_alt_jumps.push_back(jump_offset);
   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_set()
{
   static const char *incomplete_message =
      "Character set declaration starting with [ terminated prematurely - either no ] was found or the set had no content.";
   ++m_position;

   if (m_position == m_end) {
      fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
      return false;
   }
   basic_char_set<charT, traits> char_set;

   const typename traits::string_type::const_iterator base = m_position;       // where the '[' was
   typename traits::string_type::const_iterator item_base = m_position;  // where the '[' or '^' was

   while (m_position != m_end) {
      switch (this->m_traits.syntax_type(*m_position)) {
         case regex_constants::syntax_caret:
            if (m_position == base) {
               char_set.negate();
               ++m_position;
               item_base = m_position;
            } else {
               parse_set_literal(char_set);
            }
            break;

         case regex_constants::syntax_close_set:
            if (m_position == item_base) {
               parse_set_literal(char_set);
               break;

            } else {
               ++m_position;

               if (this->append_set(char_set) == nullptr) {
                  fail(regex_constants::error_ctype, m_position - m_base);
                  return false;
               }
            }
            return true;

         case regex_constants::syntax_open_set:
            if (parse_inner_set(char_set)) {
               break;
            }
            return true;

         case regex_constants::syntax_escape: {

            // look ahead and see if this is a character class shortcut
            // \d \w \s etc...

            ++m_position;

            if (this->m_traits.escape_syntax_type(*m_position) == regex_constants::escape_type_class) {
               char_class_type m = this->m_traits.lookup_classname(m_position, m_position + 1);

               if (m != 0) {
                  char_set.add_class(m);
                  ++m_position;
                  break;
               }

            } else if (this->m_traits.escape_syntax_type(*m_position) == regex_constants::escape_type_not_class) {
               // negated character class:
               char_class_type m = this->m_traits.lookup_classname(m_position, m_position + 1);

               if (m != 0) {
                  char_set.add_negated_class(m);
                  ++m_position;
                  break;
               }
            }

            // not a character class, just a regular escape:
            --m_position;
            parse_set_literal(char_set);
            break;
         }

         default:
            parse_set_literal(char_set);
            break;
      }
   }
   return m_position != m_end;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_inner_set(basic_char_set<charT, traits> &char_set)
{
   static const char *incomplete_message =
      "Character class declaration starting with [ terminated prematurely - either no ] was found or the set had no content.";
   //
   // we have either a character class [:name:]
   // a collating element [.name.]
   // or an equivalence class [=name=]
   //
   if (m_end == ++m_position) {
      fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
      return false;
   }
   switch (this->m_traits.syntax_type(*m_position)) {
      case regex_constants::syntax_dot:
         //
         // a collating element is treated as a literal:
         //
         --m_position;
         parse_set_literal(char_set);
         return true;
      case regex_constants::syntax_colon: {
         // check that character classes are actually enabled:
         if ((this->flags() & (regbase::main_option_type | regbase::no_char_classes)) == (regbase::basic_syntax_group  | regbase::no_char_classes)) {
            --m_position;
            parse_set_literal(char_set);
            return true;
         }
         // skip the ':'
         if (m_end == ++m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         typename traits::string_type::const_iterator name_first = m_position;

         // skip at least one character, then find the matching ':]'
         if (m_end == ++m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }
         while ((m_position != m_end)
                && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_colon)) {
            ++m_position;
         }

         const typename traits::string_type::const_iterator name_last = m_position;
         if (m_end == m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         if ((m_end == ++m_position)
               || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_set)) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }


         // check for negated class:
         //
         bool negated = false;
         if (this->m_traits.syntax_type(*name_first) == regex_constants::syntax_caret) {
            ++name_first;
            negated = true;
         }

         using m_type = typename traits::char_class_type;
         m_type m = this->m_traits.lookup_classname(name_first, name_last);

         if (m == 0) {
            if (char_set.empty() && (name_last - name_first == 1)) {
               // maybe a special case:
               ++m_position;
               if ( (m_position != m_end)
                     && (this->m_traits.syntax_type(*m_position)
                         == regex_constants::syntax_close_set)) {
                  if (this->m_traits.escape_syntax_type(*name_first)
                        == regex_constants::escape_type_left_word) {
                     ++m_position;
                     this->append_state(syntax_element_word_start);
                     return false;
                  }
                  if (this->m_traits.escape_syntax_type(*name_first)
                        == regex_constants::escape_type_right_word) {
                     ++m_position;
                     this->append_state(syntax_element_word_end);
                     return false;
                  }
               }
            }
            fail(regex_constants::error_ctype, name_first - m_base);
            return false;
         }
         if (negated == false) {
            char_set.add_class(m);
         } else {
            char_set.add_negated_class(m);
         }
         ++m_position;
         break;
      }
      case regex_constants::syntax_equal: {
         // skip the '='
         if (m_end == ++m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         const typename traits::string_type::const_iterator name_first = m_position;
         // skip at least one character, then find the matching '=]'
         if (m_end == ++m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_equal)) {
            ++m_position;
         }

         const typename traits::string_type::const_iterator name_last = m_position;
         if (m_end == m_position) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         if ((m_end == ++m_position)
               || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_set)) {
            fail(regex_constants::error_brack, m_position - m_base, incomplete_message);
            return false;
         }

         string_type m = this->m_traits.lookup_collatename(name_first, name_last);
         if ((0 == m.size()) || (m.size() > 2)) {
            fail(regex_constants::error_collate, name_first - m_base);
            return false;
         }
         digraph<charT> d;
         d.first = m[0];
         if (m.size() > 1) {
            d.second = m[1];
         } else {
            d.second = 0;
         }
         char_set.add_equivalent(d);
         ++m_position;
         break;
      }
      default:
         --m_position;
         parse_set_literal(char_set);
         break;
   }
   return true;
}

template <class charT, class traits>
void basic_regex_parser<charT, traits>::parse_set_literal(basic_char_set<charT, traits> &char_set)
{
   digraph<charT> start_range(get_next_set_literal(char_set));
   if (m_end == m_position) {
      fail(regex_constants::error_brack, m_position - m_base);
      return;
   }
   if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_dash) {
      // we have a range:
      if (m_end == ++m_position) {
         fail(regex_constants::error_brack, m_position - m_base);
         return;
      }
      if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_set) {
         digraph<charT> end_range = get_next_set_literal(char_set);
         char_set.add_range(start_range, end_range);
         if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_dash) {
            if (m_end == ++m_position) {
               fail(regex_constants::error_brack, m_position - m_base);
               return;
            }
            if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_close_set) {
               // trailing - :
               --m_position;
               return;
            }
            fail(regex_constants::error_range, m_position - m_base);
            return;
         }
         return;
      }
      --m_position;
   }
   char_set.add_single(start_range);
}

template <class charT, class traits>
digraph<charT> basic_regex_parser<charT, traits>::get_next_set_literal(basic_char_set<charT, traits> &char_set)
{
   digraph<charT> result;
   switch (this->m_traits.syntax_type(*m_position)) {
      case regex_constants::syntax_dash:
         if (!char_set.empty()) {
            // see if we are at the end of the set:
            if ((++m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_set)) {
               fail(regex_constants::error_range, m_position - m_base);
               return result;
            }
            --m_position;
         }
         result.first = *m_position++;
         return result;
      case regex_constants::syntax_escape:
         // check to see if escapes are supported first:
         if (this->flags() & regex_constants::no_escape_in_lists) {
            result = *m_position++;
            break;
         }
         ++m_position;
         result = unescape_character();
         break;
      case regex_constants::syntax_open_set: {
         if (m_end == ++m_position) {
            fail(regex_constants::error_collate, m_position - m_base);
            return result;
         }
         if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_dot) {
            --m_position;
            result.first = *m_position;
            ++m_position;
            return result;
         }
         if (m_end == ++m_position) {
            fail(regex_constants::error_collate, m_position - m_base);
            return result;
         }

         const typename traits::string_type::const_iterator name_first = m_position;
         // skip at least one character, then find the matching ':]'
         if (m_end == ++m_position) {
            fail(regex_constants::error_collate, name_first - m_base);
            return result;
         }

         while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_dot)) {
            ++m_position;
         }

         const typename traits::string_type::const_iterator name_last = m_position;
         if (m_end == m_position) {
            fail(regex_constants::error_collate, name_first - m_base);
            return result;
         }
         if ((m_end == ++m_position)
               || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_set)) {
            fail(regex_constants::error_collate, name_first - m_base);
            return result;
         }
         ++m_position;
         string_type s = this->m_traits.lookup_collatename(name_first, name_last);
         if (s.empty() || (s.size() > 2)) {
            fail(regex_constants::error_collate, name_first - m_base);
            return result;
         }
         result.first = s[0];
         if (s.size() > 1) {
            result.second = s[1];
         } else {
            result.second = 0;
         }
         return result;
      }
      default:
         result = *m_position++;
   }
   return result;
}

//
// does a value fit in the specified charT type?
//
template <class charT>
constexpr bool valid_value(charT, intmax_t v)
{
   return (v >> (sizeof(charT) * CHAR_BIT)) == 0;
}


template <class charT, class traits>
charT basic_regex_parser<charT, traits>::unescape_character()
{
   charT result(0);

   if (m_position == m_end) {
      fail(regex_constants::error_escape, m_position - m_base, "Escape sequence terminated prematurely.");
      return false;
   }

   switch (this->m_traits.escape_syntax_type(*m_position)) {

      case regex_constants::escape_type_control_a:
         result = charT('\a');
         break;

      case regex_constants::escape_type_e:
         result = charT(27);
         break;

      case regex_constants::escape_type_control_f:
         result = charT('\f');
         break;

      case regex_constants::escape_type_control_n:
         result = charT('\n');
         break;

      case regex_constants::escape_type_control_r:
         result = charT('\r');
         break;

      case regex_constants::escape_type_control_t:
         result = charT('\t');
         break;

      case regex_constants::escape_type_control_v:
         result = charT('\v');
         break;

      case regex_constants::escape_type_word_assert:
         result = charT('\b');
         break;

      case regex_constants::escape_type_hex:
         ++m_position;

         if (m_position == m_end) {
            // Rewind to start of escape:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
               --m_position;
            }

            fail(regex_constants::error_escape, m_position - m_base, "Hexadecimal escape sequence terminated prematurely.");
            return result;
         }

         // maybe have \x{ddd}
         if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_open_brace) {
            ++m_position;

            if (m_position == m_end) {
               // Rewind to start of escape:
               --m_position;

               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
                  --m_position;
               }

               fail(regex_constants::error_escape, m_position - m_base, "Missing } in hexadecimal escape sequence.");
               return result;
            }

            intmax_t i = this->m_traits.toi(m_position, m_end, 16);

            if ((m_position == m_end) || (i < 0) ||
                  (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_brace)) {
               // Rewind to start of escape:
               --m_position;

               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
                  --m_position;
               }

               fail(regex_constants::error_badbrace, m_position - m_base, "Hexadecimal escape sequence was invalid.");
               return result;
            }

            ++m_position;

            result = charT(static_cast<char32_t>(i));

         } else  {
            std::ptrdiff_t len = (std::min)(static_cast<std::ptrdiff_t>(2), static_cast<std::ptrdiff_t>(m_end - m_position));
            intmax_t i = this->m_traits.toi(m_position, m_position + len, 16);

            if ((i < 0) || ! valid_value(charT(0), i)) {
               // Rewind to start of escape:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
                  --m_position;
               }
               fail(regex_constants::error_escape, m_position - m_base, "Escape sequence did not encode a valid character.");
               return result;
            }

            result = charT(static_cast<char32_t>(i));
         }

         return result;

      case regex_constants::syntax_digit: {
         // an octal escape sequence, first character must be a zero followed by up to 3 octal digits

         std::ptrdiff_t len = (std::min)(static_cast<std::ptrdiff_t>(std::distance(m_position, m_end)), static_cast<std::ptrdiff_t>(4));
         typename traits::string_type::const_iterator bp = m_position;

         intmax_t val = this->m_traits.toi(bp, bp + 1, 8);

         if (val != 0) {
            // Rewind to start of escape:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
               --m_position;
            }

            // not an octal escape
            fail(regex_constants::error_escape, m_position - m_base, "Invalid octal escape sequence.");
            return result;
         }

         val = this->m_traits.toi(m_position, m_position + len, 8);

         if (val < 0) {
            // Rewind to start of escape:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
               --m_position;
            }

            fail(regex_constants::error_escape, m_position - m_base, "Octal escape sequence is invalid.");
            return result;
         }

         return charT(static_cast<char32_t>(val));
      }

      case regex_constants::escape_type_named_char: {
         ++m_position;

         if (m_position == m_end) {
            // Rewind to start of escape:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
               --m_position;
            }
            fail(regex_constants::error_escape, m_position - m_base);
            return false;
         }

         // maybe have \N{name}
         if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_open_brace) {
            typename traits::string_type::const_iterator base = m_position;

            // skip forward until we find enclosing brace:
            while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_brace)) {
               ++m_position;
            }
            if (m_position == m_end) {
               // Rewind to start of escape:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
                  --m_position;
               }
               fail(regex_constants::error_escape, m_position - m_base);
               return false;
            }

            string_type s = this->m_traits.lookup_collatename(++base, m_position++);
            if (s.empty()) {
               // Rewind to start of escape:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
                  --m_position;
               }
               fail(regex_constants::error_collate, m_position - m_base);
               return false;
            }
            if (s.size() == 1) {
               return s[0];
            }
         }

         // failure, rewind to start of escape
         --m_position;

         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
            --m_position;
         }

         fail(regex_constants::error_escape, m_position - m_base);
         return false;
      }

      default:
         result = *m_position;
         break;
   }

   ++m_position;
   return result;

}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_backref()
{
   assert(m_position != m_end);

   typename traits::string_type::const_iterator pc = m_position;

   intmax_t i = this->m_traits.toi(pc, pc + 1, 10);

   if ((i == 0) || (((this->flags() & regbase::main_option_type) == regbase::perl_syntax_group) && (this->flags() & regbase::no_bk_refs))) {
      // not a backref at all but an octal escape sequence:
      charT c = unescape_character();
      this->append_literal(c);
   }

   else if ((i > 0) && (this->m_backrefs & (1u << (i - 1)))) {
      m_position = pc;
      re_brace *pb = static_cast<re_brace *>(this->append_state(syntax_element_backref, sizeof(re_brace)));
      pb->index = i;
      pb->icase = this->flags() & regbase::icase;
   } else {
      // Rewind to start of escape:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
         --m_position;
      }
      fail(regex_constants::error_backref, m_position - m_base);
      return false;
   }
   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_QE()
{
   // parse a \Q...\E sequence:

   ++m_position; // skip the Q

   typename traits::string_type::const_iterator start = m_position;
   typename traits::string_type::const_iterator end;

   do {
      while ((m_position != m_end)
             && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape)) {
         ++m_position;
      }
      if (m_position == m_end) {
         //  a \Q...\E sequence may terminate with the end of the expression:
         end = m_position;
         break;
      }
      if (++m_position == m_end) { // skip the escape
         fail(regex_constants::error_escape, m_position - m_base, "Unterminated \\Q...\\E sequence.");
         return false;
      }
      // check to see if it's a \E:
      if (this->m_traits.escape_syntax_type(*m_position) == regex_constants::escape_type_E) {
         ++m_position;
         end = m_position - 2;
         break;
      }
      // otherwise go round again:
   } while (true);
   //
   // now add all the character between the two escapes as literals:
   //
   while (start != end) {
      this->append_literal(*start);
      ++start;
   }

   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_perl_extension()
{
   if (++m_position == m_end) {
      // Rewind to start of (? sequence:
      --m_position;

      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
         --m_position;
      }

      fail(regex_constants::error_perl_extension, m_position - m_base);
      return false;
   }

   // treat comments as a special case, as these are the only ones that do not
   // start with a leading startmark state

   if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_hash) {

      while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position++) != regex_constants::syntax_close_mark))
      {
         // no code should appear here
      }

      return true;
   }

   // backup some state, and prepare the way

   int markid = 0;
   std::ptrdiff_t jump_offset = 0;

   re_brace *pb = static_cast<re_brace *>(this->append_state(syntax_element_startmark, sizeof(re_brace)));
   pb->icase    = this->flags() & regbase::icase;
   std::ptrdiff_t last_paren_start = this->getoffset(pb);

   // back up insertion point for alternations, and set new point:
   std::ptrdiff_t last_alt_point = m_alt_insert_point;
   this->m_pdata->m_data.align();

   m_alt_insert_point = this->m_pdata->m_data.size();
   std::ptrdiff_t expected_alt_point = m_alt_insert_point;

   bool restore_flags = true;
   regex_constants::syntax_option_type old_flags = this->flags();

   bool old_case_change = m_has_case_change;
   m_has_case_change = false;
   charT name_delim;

   int mark_reset = m_mark_reset;
   int max_mark = m_max_mark;

   m_mark_reset = -1;
   m_max_mark = m_mark_count;
   intmax_t v;

   // select the actual extension used
   switch (this->m_traits.syntax_type(*m_position)) {

      case regex_constants::syntax_or:
         m_mark_reset = m_mark_count;
         [[fallthrough]];

      case regex_constants::syntax_colon:
         // a non-capturing mark:

         pb->index = markid = 0;
         ++m_position;
         break;

      case regex_constants::syntax_digit: {
         // a recursive subexpression

         v = this->m_traits.toi(m_position, m_end, 10);

         if ((v < 0) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (? sequence:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }

            fail(regex_constants::error_perl_extension, m_position - m_base,
                 "The recursive sub-expression refers to an invalid marking group, or is unterminated.");
            return false;
         }

      insert_recursion:
         pb->index = markid = 0;
         re_recurse *pr = static_cast<re_recurse *>(this->append_state(syntax_element_recurse, sizeof(re_recurse)));
         pr->alt.i = v;
         pr->state_id = 0;

         static_cast<re_case *>( this->append_state(syntax_element_toggle_case,
                  sizeof(re_case)))->icase = this->flags() & regbase::icase;

         break;
      }

      case regex_constants::syntax_plus:
         // A forward-relative recursive subexpression:

         ++m_position;
         v = this->m_traits.toi(m_position, m_end, 10);

         if ((v <= 0) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base, "An invalid or unterminated recursive sub-expression.");
            return false;
         }

         if ((std::numeric_limits<intmax_t>::max)() - m_mark_count < v) {
            fail(regex_constants::error_perl_extension, m_position - m_base, "An invalid or unterminated recursive sub-expression.");
            return false;
         }

         v += m_mark_count;
         goto insert_recursion;

      case regex_constants::syntax_dash:

         // Possibly a backward-relative recursive subexpression:

         ++m_position;
         v = this->m_traits.toi(m_position, m_end, 10);
         if (v <= 0) {
            --m_position;
            // Oops not a relative recursion at all, but a (?-imsx) group:
            goto option_group_jump;
         }

         v = m_mark_count + 1 - v;
         if (v <= 0) {
            // Rewind to start of (? sequence:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base, "An invalid or unterminated recursive sub-expression.");
            return false;
         }

         goto insert_recursion;

      case regex_constants::syntax_equal:
         pb->index = markid = -1;
         ++m_position;
         jump_offset = this->getoffset(this->append_state(syntax_element_jump, sizeof(re_jump)));
         this->m_pdata->m_data.align();
         m_alt_insert_point = this->m_pdata->m_data.size();
         break;

      case regex_constants::syntax_not:
         pb->index = markid = -2;
         ++m_position;
         jump_offset = this->getoffset(this->append_state(syntax_element_jump, sizeof(re_jump)));
         this->m_pdata->m_data.align();
         m_alt_insert_point = this->m_pdata->m_data.size();
         break;

      case regex_constants::escape_type_left_word: {
         // a lookbehind assertion

         if (++m_position == m_end) {
            // Rewind to start of (? sequence:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }

            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }

         regex_constants::syntax_type t = this->m_traits.syntax_type(*m_position);

         if (t == regex_constants::syntax_not) {
            pb->index = markid = -2;

         } else if (t == regex_constants::syntax_equal) {
            pb->index = markid = -1;

         } else {
            // probably a named capture which also starts (?<
            name_delim = '>';
            --m_position;

            goto named_capture_jump;
         }

         ++m_position;
         jump_offset = this->getoffset(this->append_state(syntax_element_jump, sizeof(re_jump)));
         this->append_state(syntax_element_backstep, sizeof(re_brace));
         this->m_pdata->m_data.align();
         m_alt_insert_point = this->m_pdata->m_data.size();
         break;
      }

      case regex_constants::escape_type_right_word:
         //
         // an independent sub-expression:
         //
         pb->index = markid = -3;
         ++m_position;
         jump_offset = this->getoffset(this->append_state(syntax_element_jump, sizeof(re_jump)));
         this->m_pdata->m_data.align();
         m_alt_insert_point = this->m_pdata->m_data.size();
         break;
      case regex_constants::syntax_open_mark: {
         // a conditional expression:
         pb->index = markid = -4;
         if (++m_position == m_end) {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         v = this->m_traits.toi(m_position, m_end, 10);
         if (m_position == m_end) {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         if (*m_position == charT('R')) {
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }

            if (*m_position == charT('&')) {
               const typename traits::string_type::const_iterator base = ++m_position;

               while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
                  ++m_position;
               }

               if (m_position == m_end) {
                  // Rewind to start of (? sequence:
                  --m_position;
                  while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                     --m_position;
                  }
                  fail(regex_constants::error_perl_extension, m_position - m_base);
                  return false;
               }

               v = -static_cast<int>(hash_value_from_capture_name(base, m_position));
            } else {
               v = -this->m_traits.toi(m_position, m_end, 10);
            }
            re_brace *br = static_cast<re_brace *>(this->append_state(syntax_element_assert_backref, sizeof(re_brace)));
            br->index = v < 0 ? (v - 1) : 0;
            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
         } else if ((*m_position == charT('\'')) || (*m_position == charT('<'))) {
            const typename traits::string_type::const_iterator base = ++m_position;

            while ((m_position != m_end) && (*m_position != charT('>')) && (*m_position != charT('\''))) {
               ++m_position;
            }

            if (m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            v = static_cast<int>(hash_value_from_capture_name(base, m_position));
            re_brace *br = static_cast<re_brace *>(this->append_state(syntax_element_assert_backref, sizeof(re_brace)));
            br->index = v;
            if (((*m_position != charT('>')) && (*m_position != charT('\''))) || (++m_position == m_end)) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base, "Unterminated named capture.");
               return false;
            }
            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
         } else if (*m_position == charT('D')) {
            const char *def = "DEFINE";

            while (*def && (m_position != m_end) && (*m_position == charT(*def))) {
               ++m_position, ++def;
            }

            if ((m_position == m_end) || *def) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            re_brace *br = static_cast<re_brace *>(this->append_state(syntax_element_assert_backref, sizeof(re_brace)));
            br->index = 9999; // special magic value!
            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
         } else if (v > 0) {
            re_brace *br = static_cast<re_brace *>(this->append_state(syntax_element_assert_backref, sizeof(re_brace)));
            br->index = v;
            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
         } else {
            // verify that we have a lookahead or lookbehind assert:
            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_question) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (++m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            if (this->m_traits.syntax_type(*m_position) == regex_constants::escape_type_left_word) {
               if (++m_position == m_end) {
                  // Rewind to start of (? sequence:
                  --m_position;
                  while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                     --m_position;
                  }
                  fail(regex_constants::error_perl_extension, m_position - m_base);
                  return false;
               }
               if ((this->m_traits.syntax_type(*m_position) != regex_constants::syntax_equal)
                     && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_not)) {
                  // Rewind to start of (? sequence:
                  --m_position;
                  while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                     --m_position;
                  }
                  fail(regex_constants::error_perl_extension, m_position - m_base);
                  return false;
               }
               m_position -= 3;
            } else {
               if ((this->m_traits.syntax_type(*m_position) != regex_constants::syntax_equal)
                     && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_not)) {
                  // Rewind to start of (? sequence:
                  --m_position;
                  while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                     --m_position;
                  }
                  fail(regex_constants::error_perl_extension, m_position - m_base);
                  return false;
               }
               m_position -= 2;
            }
         }
         break;
      }
      case regex_constants::syntax_close_mark:
         // Rewind to start of (? sequence:
         --m_position;

         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }

         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;

      case regex_constants::escape_type_end_buffer: {
         name_delim = *m_position;

      named_capture_jump:
         markid = 0;

         if (0 == (this->flags() & regbase::nosubs)) {
            markid = ++m_mark_count;

            if (this->flags() & regbase::save_subexpression_location) {
               this->m_pdata->m_subs.push_back(std::pair<std::size_t, std::size_t>(std::distance(m_base, m_position) - 2, 0));
            }

         }

         pb->index = markid;
         const typename traits::string_type::const_iterator base = ++m_position;

         if (m_position == m_end) {
            // Rewind to start of (? sequence:
            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }

            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }

         while ((m_position != m_end) && (*m_position != name_delim)) {
            ++m_position;
         }


         if (m_position == m_end) {
            // Rewind to start of (? sequence

            --m_position;

            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }

            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }

         this->m_pdata->set_name(base, m_position, markid);
         ++m_position;

         break;
      }

      default:
         if (*m_position == charT('R')) {
            ++m_position;
            v = 0;

            if (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            goto insert_recursion;
         }
         if (*m_position == charT('&')) {
            ++m_position;
            const typename traits::string_type::const_iterator base = m_position;
            while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
               ++m_position;
            }
            if (m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }
            v = static_cast<int>(hash_value_from_capture_name(base, m_position));
            goto insert_recursion;
         }
         if (*m_position == charT('P')) {
            ++m_position;
            if (m_position == m_end) {
               // Rewind to start of (? sequence:
               --m_position;
               while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                  --m_position;
               }
               fail(regex_constants::error_perl_extension, m_position - m_base);
               return false;
            }

            if (*m_position == charT('>')) {
               ++m_position;
               const typename traits::string_type::const_iterator base = m_position;

               while ((m_position != m_end) && (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
                  ++m_position;
               }

               if (m_position == m_end) {
                  // Rewind to start of (? sequence:
                  --m_position;
                  while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
                     --m_position;
                  }
                  fail(regex_constants::error_perl_extension, m_position - m_base);
                  return false;
               }

               v = static_cast<int>(hash_value_from_capture_name(base, m_position));
               goto insert_recursion;
            }
         }
         //
         // lets assume that we have a (?imsx) group and try and parse it:
         //
      option_group_jump:
         regex_constants::syntax_option_type opts = parse_options();
         if (m_position == m_end) {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         // make a note of whether we have a case change:
         m_has_case_change = ((opts & regbase::icase) != (this->flags() & regbase::icase));
         pb->index = markid = 0;

         if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_close_mark) {
            // update flags and carry on as normal:
            this->flags(opts);
            restore_flags = false;
            old_case_change |= m_has_case_change; // defer end of scope by one ')'
         } else if (this->m_traits.syntax_type(*m_position) == regex_constants::syntax_colon) {
            // update flags and carry on until the matching ')' is found:
            this->flags(opts);
            ++m_position;
         } else {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }

         // finally append a case change state if we need it:
         if (m_has_case_change) {
            static_cast<re_case *>(
               this->append_state(syntax_element_toggle_case, sizeof(re_case))
            )->icase = opts & regbase::icase;
         }

   }

   // now recursively add more states, this will terminate when we get to a matching ')'
   parse_all();

   // Unwind alternatives:

   if (0 == unwind_alts(last_paren_start)) {
      // Rewind to start of (? sequence:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
         --m_position;
      }
      fail(regex_constants::error_perl_extension, m_position - m_base, "Invalid alternation operators within (?...) block.");
      return false;
   }

   // we either have a ')' or we have run out of characters prematurely:

   if (m_position == m_end) {
      // Rewind to start of (? sequence:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
         --m_position;
      }
      this->fail(regex_constants::error_paren, std::distance(m_base, m_end));
      return false;
   }

   assert(this->m_traits.syntax_type(*m_position) == regex_constants::syntax_close_mark);
   ++m_position;
   //
   // restore the flags:
   //
   if (restore_flags) {
      // append a case change state if we need it:
      if (m_has_case_change) {
         static_cast<re_case *>(
            this->append_state(syntax_element_toggle_case, sizeof(re_case))
         )->icase = old_flags & regbase::icase;
      }
      this->flags(old_flags);
   }
   //
   // set up the jump pointer if we have one:
   //
   if (jump_offset) {
      this->m_pdata->m_data.align();
      re_jump *jmp = static_cast<re_jump *>(this->getaddress(jump_offset));
      jmp->alt.i = this->m_pdata->m_data.size() - this->getoffset(jmp);
      if ((this->m_last_state == jmp) && (markid != -2)) {
         // Oops... we didn't have anything inside the assertion.
         // Note we don't get here for negated forward lookahead as (?!)
         // does have some uses.
         // Rewind to start of (? sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base, "Invalid or empty zero width assertion.");
         return false;
      }
   }

   // verify that if this is conditional expression, that we do have
   // an alternative, if not add one:

   if (markid == -4) {

      re_syntax_base *b = this->getaddress(expected_alt_point);
      // Make sure we have exactly one alternative following this state:
      if (b->type != syntax_element_alt) {
         re_alt *alt = static_cast<re_alt *>(this->insert_state(expected_alt_point, syntax_element_alt, sizeof(re_alt)));
         alt->alt.i = this->m_pdata->m_data.size() - this->getoffset(alt);

      } else if (((std::ptrdiff_t)this->m_pdata->m_data.size() > (static_cast<re_alt *>(b)->alt.i + this->getoffset(b))) &&
                 (static_cast<re_alt *>(b)->alt.i > 0) && this->getaddress(static_cast<re_alt *>(b)->alt.i, b)->type == syntax_element_alt) {

         // Can not have seen more than one alternative
         // Rewind to start of (? sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }

         fail(regex_constants::error_bad_pattern, m_position - m_base,
              "More than one alternation operator | was encountered inside a conditional expression.");
         return false;

      } else {
         // We must *not* have seen an alternative inside a (DEFINE) block:
         b = this->getaddress(b->next.i, b);
         if ((b->type == syntax_element_assert_backref) && (static_cast<re_brace *>(b)->index == 9999)) {
            // Rewind to start of (? sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_bad_pattern, m_position - m_base, "Alternation operators are not allowed inside a DEFINE block.");
            return false;
         }
      }
      // check for invalid repetition of next state:
      b = this->getaddress(expected_alt_point);
      b = this->getaddress(static_cast<re_alt *>(b)->next.i, b);
      if ((b->type != syntax_element_assert_backref)
            && (b->type != syntax_element_startmark)) {
         // Rewind to start of (? sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_badrepeat, m_position - m_base, "A repetition operator cannot be applied to a zero-width assertion.");
         return false;
      }
   }
   //
   // append closing parenthesis state:
   //
   pb = static_cast<re_brace *>(this->append_state(syntax_element_endmark, sizeof(re_brace)));
   pb->index = markid;
   pb->icase = this->flags() & regbase::icase;
   this->m_paren_start = last_paren_start;
   //
   // restore the alternate insertion point:
   //
   this->m_alt_insert_point = last_alt_point;
   //
   // and the case change data:
   //
   m_has_case_change = old_case_change;
   //
   // And the mark_reset data:
   //
   if (m_max_mark > m_mark_count) {
      m_mark_count = m_max_mark;
   }
   m_mark_reset = mark_reset;
   m_max_mark = max_mark;


   if (markid > 0) {
      if (this->flags() & regbase::save_subexpression_location) {
         this->m_pdata->m_subs.at(markid - 1).second = std::distance(m_base, m_position) - 1;
      }

      //
      // allow backrefs to this mark:
      //
      if (markid < (int)(sizeof(unsigned) * CHAR_BIT)) {
         this->m_backrefs |= 1u << (markid - 1);
      }
   }
   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::match_verb(const char *verb)
{
   while (*verb) {
      if (static_cast<charT>(*verb) != *m_position) {
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }
      if (++m_position == m_end) {
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }
      ++verb;
   }
   return true;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::parse_perl_verb()
{
   if (++m_position == m_end) {
      // Rewind to start of (* sequence:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
         --m_position;
      }
      fail(regex_constants::error_perl_extension, m_position - m_base);
      return false;
   }

   const auto tmp = *m_position;

   if (tmp == 'F') {
      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if ((this->m_traits.syntax_type(*m_position) == regex_constants::syntax_close_mark) || match_verb("AIL")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         ++m_position;
         this->append_state(syntax_element_fail);
         return true;
      }

   } else if (tmp == 'A') {
      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if (match_verb("CCEPT")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         ++m_position;
         this->append_state(syntax_element_accept);
         return true;
      }

   } else if (tmp == 'C') {
      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if (match_verb("OMMIT")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         ++m_position;
         static_cast<re_commit *>(this->append_state(syntax_element_commit, sizeof(re_commit)))->action = commit_commit;
         this->m_pdata->m_disable_match_any = true;
         return true;
      }

   } else if (tmp == 'P') {

      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if (match_verb("RUNE")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }

         ++m_position;
         static_cast<re_commit *>(this->append_state(syntax_element_commit, sizeof(re_commit)))->action = commit_prune;
         this->m_pdata->m_disable_match_any = true;
         return true;
      }

   } else if (tmp == 'S') {
      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if (match_verb("KIP")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         ++m_position;
         static_cast<re_commit *>(this->append_state(syntax_element_commit, sizeof(re_commit)))->action = commit_skip;
         this->m_pdata->m_disable_match_any = true;
         return true;
      }

   } else if (tmp == 'T') {
      if (++m_position == m_end) {
         // Rewind to start of (* sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }
         fail(regex_constants::error_perl_extension, m_position - m_base);
         return false;
      }

      if (match_verb("HEN")) {
         if ((m_position == m_end) || (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_close_mark)) {
            // Rewind to start of (* sequence:
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_perl_extension, m_position - m_base);
            return false;
         }
         ++m_position;
         this->append_state(syntax_element_then);
         this->m_pdata->m_disable_match_any = true;
         return true;
      }

   }

   // Rewind to start of (* sequence:
   --m_position;

   while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
      --m_position;
   }

   fail(regex_constants::error_perl_extension, m_position - m_base);
   return false;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::add_emacs_code(bool negate)
{
   //
   // parses an emacs style \sx or \Sx construct.
   //
   if (++m_position == m_end) {
      // Rewind to start of sequence:
      --m_position;
      while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_escape) {
         --m_position;
      }
      fail(regex_constants::error_escape, m_position - m_base);
      return false;
   }
   basic_char_set<charT, traits> char_set;
   if (negate) {
      char_set.negate();
   }

   static const charT s_punct[5] = { 'p', 'u', 'n', 'c', 't', };

   const auto tmp = *m_position;

   if (tmp == 's' || tmp == ' ') {
      char_set.add_class(this->m_mask_space);

   } else if (tmp == 'w') {
      char_set.add_class(this->m_word_mask);

   } else if (tmp == '_') {
      char_set.add_single(digraph<charT>(charT('$')));
      char_set.add_single(digraph<charT>(charT('&')));
      char_set.add_single(digraph<charT>(charT('*')));
      char_set.add_single(digraph<charT>(charT('+')));
      char_set.add_single(digraph<charT>(charT('-')));
      char_set.add_single(digraph<charT>(charT('_')));
      char_set.add_single(digraph<charT>(charT('<')));
      char_set.add_single(digraph<charT>(charT('>')));

   } else if (tmp == '.') {
      char_set.add_class(this->m_traits.lookup_classname(s_punct, s_punct + 5));

   } else if (tmp == '(') {
      char_set.add_single(digraph<charT>(charT('(')));
      char_set.add_single(digraph<charT>(charT('[')));
      char_set.add_single(digraph<charT>(charT('{')));

   } else if (tmp == ')') {
      char_set.add_single(digraph<charT>(charT(')')));
      char_set.add_single(digraph<charT>(charT(']')));
      char_set.add_single(digraph<charT>(charT('}')));

   } else if (tmp == '"') {
      char_set.add_single(digraph<charT>(charT('"')));
      char_set.add_single(digraph<charT>(charT('\'')));
      char_set.add_single(digraph<charT>(charT('`')));

   } else if (tmp == '\'') {
      char_set.add_single(digraph<charT>(charT('\'')));
      char_set.add_single(digraph<charT>(charT(',')));
      char_set.add_single(digraph<charT>(charT('#')));

   } else if (tmp == '<') {
      char_set.add_single(digraph<charT>(charT(';')));

   } else if (tmp == '>') {
      char_set.add_single(digraph<charT>(charT('\n')));
      char_set.add_single(digraph<charT>(charT('\f')));

   } else {
      fail(regex_constants::error_ctype, m_position - m_base);
      return false;
   }

   if (this->append_set(char_set) == nullptr) {
      fail(regex_constants::error_ctype, m_position - m_base);
      return false;
   }

   ++m_position;

   return true;
}

template <class charT, class traits>
regex_constants::syntax_option_type basic_regex_parser<charT, traits>::parse_options()
{
   // we have a (?imsx-imsx) group, convert it into a set of flags:
   regex_constants::syntax_option_type f = this->flags();
   bool breakout = false;

   do {
      const auto tmp = *m_position;

      if (tmp == 's') {
         f |= regex_constants::mod_s;
         f &= ~regex_constants::no_mod_s;

      } else if (tmp == 'm') {
         f &= ~regex_constants::no_mod_m;

      } else if (tmp == 'i') {
         f |= regex_constants::icase;

      } else if (tmp == 'x') {
         f |= regex_constants::mod_x;

      } else {
         breakout = true;
         continue;
      }

      if (++m_position == m_end) {
         // Rewind to start of (? sequence:
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }

         fail(regex_constants::error_paren, m_position - m_base);
         return false;
      }

   } while (! breakout);

   breakout = false;

   if (*m_position == static_cast<charT>('-')) {
      if (++m_position == m_end) {
         // Rewind to start of (? sequence
         --m_position;
         while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
            --m_position;
         }

         fail(regex_constants::error_paren, m_position - m_base);
         return false;
      }

      do {
         const auto tmp = *m_position;

         if (tmp == 's') {
            f &= ~regex_constants::mod_s;
            f |= regex_constants::no_mod_s;

         } else if (tmp == 'n') {
            f |= regex_constants::no_mod_m;

         } else if (tmp == 'i') {
            f &= ~regex_constants::icase;

         } else if (tmp == 'x') {
            f &= ~regex_constants::mod_x;

         } else {
            breakout = true;
            continue;
         }

         if (++m_position == m_end) {
            // Rewind to start of (? sequence
            --m_position;
            while (this->m_traits.syntax_type(*m_position) != regex_constants::syntax_open_mark) {
               --m_position;
            }
            fail(regex_constants::error_paren, m_position - m_base);
            return false;
         }

      } while (!breakout);
   }

   return f;
}

template <class charT, class traits>
bool basic_regex_parser<charT, traits>::unwind_alts(std::ptrdiff_t last_paren_start)
{
   // If we didn't actually add any states after the last
   // alternative then that's an error:

   if ((this->m_alt_insert_point == static_cast<std::ptrdiff_t>(this->m_pdata->m_data.size()))
         && m_alt_jumps.size() && (m_alt_jumps.back() > last_paren_start) &&
         ! ( ((this->flags() & regbase::main_option_type) == regbase::perl_syntax_group) &&
            ((this->flags() & regbase::no_empty_expressions) == 0)) ) {

      fail(regex_constants::error_empty, this->m_position - this->m_base,
            "Can not terminate a sub-expression with an alternation operator |.");

      return false;
   }

   // Fix our alternatives
   while (m_alt_jumps.size() && (m_alt_jumps.back() > last_paren_start)) {

      // fix the jump to point to the end of the states that were just added
      std::ptrdiff_t jump_offset = m_alt_jumps.back();

      m_alt_jumps.pop_back();
      this->m_pdata->m_data.align();
      re_jump *jmp = static_cast<re_jump *>(this->getaddress(jump_offset));

      assert(jmp->type == syntax_element_jump);
      jmp->alt.i = this->m_pdata->m_data.size() - jump_offset;
   }

   return true;
}

}   // end namespace

}   // end namespace

#endif
