/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <tuple>
#include <vector>

#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant.hpp>

#include "common/common_types.h"
#include "frontend_arm/ir/opcodes.h"

#include "print_tuples.h"

using Opcode = Dynarmic::IR::Opcode;

struct Inst;
struct Ref;
using Needle = boost::variant<Inst, Ref>;

struct Inst {
    Opcode opcode;
    std::vector<Needle> children;
    std::vector<Needle> parents;
};

struct Ref {
    std::string name;
};

int main(int argc, char** argv) {
    std::string input{R"(
        tile TestTile
            in [ a (REGISTER), b (REGISTER) ]
            out [ o?(REGISTER) ]
            match
                o: ()
            endmatch
            code
                code
            endcode
        endtile
    )"};

    std::string out_input = "out(REGISTER)";
    std::string ident_input = "out";

    using Iterator = decltype(input.begin());
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    using namespace qi::labels;

    using TileIn = std::tuple<std::string, std::string>;
    using TileOut = std::tuple<std::string, bool, std::string>;
    using TileEntry = std::tuple<std::string, std::vector<TileIn>, std::vector<TileOut>, std::string, std::string>;

    qi::rule<Iterator, std::string()> identifier;
    identifier %=
        qi::lexeme[ascii::alpha >> *ascii::alnum];

    qi::rule<Iterator, std::string()> needle_name;
    needle_name %=
        qi::lexeme['$' >> *ascii::alnum];

    qi::rule<Iterator, std::string()> until_closeparen;
    until_closeparen %=
        qi::lexeme[*(qi::char_ - ')')];

    qi::rule<Iterator, TileIn(), ascii::space_type> in;
    in %=
            (identifier >> '(' >> until_closeparen >> ')');

    qi::rule<Iterator, bool()> question_mark;
    question_mark =
            ('?' >> qi::attr(true)) | qi::attr(false);

    qi::rule<Iterator, TileOut(), ascii::space_type> out;
    out %=
            (identifier >> question_mark >> '(' >> until_closeparen >> ')');

    qi::rule<Iterator, TileEntry(), ascii::space_type> entry;
    entry %= (
        qi::lit("tile") >> identifier >>
            qi::lit("in") >> '[' >> (in % ',') >> ']' >>
            qi::lit("out") >> '[' >> (out % ',') >> ']' >>
            qi::lit("match") >>
                qi::lexeme[*(qi::char_ - qi::lit("endmatch"))] >>
            qi::lit("endmatch") >>
            qi::lit("code") >>
                qi::lexeme[*(qi::char_ - qi::lit("endcode"))] >>
            qi::lit("endcode") >>
        qi::lit("endtile")
    );

    auto first = input.begin(), last = input.end();
    TileEntry te;
    bool r = qi::phrase_parse(first, last, entry, ascii::space, te);

    if (first != last || !r) {
        std::cout << "Fail!" << std::endl;
        std::cout << te << std::endl;
    } else {
        std::cout << te << std::endl;
    }

    first = out_input.begin(), last = out_input.end();
    TileOut to;
    r = qi::phrase_parse(first, last, out, ascii::space, to);

    if (first != last || !r) {
        std::cout << "Fail!" << std::endl;
        std::cout << to << std::endl;
    } else {
        std::cout << to << std::endl;
    }


    first = ident_input.begin(), last = ident_input.end();
    std::string test_ident;
    r = qi::phrase_parse(first, last, identifier, ascii::space, test_ident);

    if (first != last || !r) {
        std::cout << "Fail!" << std::endl;
        std::cout << test_ident << std::endl;
    } else {
        std::cout << test_ident << std::endl;
    }

    return 0;
}
