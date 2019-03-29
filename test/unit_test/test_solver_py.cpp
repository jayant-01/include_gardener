// Include-Gardener
//
// Copyright (C) 2019  Christian Haettich [feddischson]
//
// This program is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation;
// either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General
// Public License along with this program; if not, see
// <http://www.gnu.org/licenses/>.
//
/*
#include <regex>

#include "graph.h"
#include "solver_py.h"
#include "solver.h"
#include "statement_detector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <sstream>

using INCLUDE_GARDENER::Solver_Py;
using INCLUDE_GARDENER::Solver;
using INCLUDE_GARDENER::Statement_Detector;
using INCLUDE_GARDENER::Vertex;
using INCLUDE_GARDENER::Edge;
using INCLUDE_GARDENER::Edge_Descriptor;

using std::stringstream;
using std::string;
using std::vector;
using std::optional;
using std::pair;
using std::istream;
using std::make_shared;
using std::endl;
using testing::StrEq;
using testing::StrNe;
using testing::Not;
using testing::HasSubstr;
using testing::Contains;
using testing::Ge;
using testing::Return;
using testing::SizeIs;
using testing::_;

using boost::filesystem::path;

class SolverPyTest : public ::testing::Test, public Solver_Py {
public:
  using Solver_Py::get_first_substring;
  using Solver_Py::dots_to_system_slash;
  using Solver_Py::convert_import_statement_to_path_str;
  using Solver_Py::how_many_directories_above;
  using Solver_Py::begins_with_dot;
  using Solver_Py::without_prepended_dots;
  using Solver_Py::remove_as_statements;
};

class MockSolver_Py : public Solver_Py {
public:
  MockSolver_Py() = default;
  MOCK_METHOD4(add_edge, void(const string &, const string &, unsigned int,
                                unsigned int));
  MOCK_METHOD1(split_comma_string, vector<string>(const string &statement));
};

// For testing Solver_Py specific functions
class MockSolver_Py2 : public Solver_Py {
public:
  MockSolver_Py2() = default;

  MOCK_METHOD1(convert_import_statement_to_path_str, string(const string &statement));
  MOCK_METHOD1(import_statement_to_path, string(const string &statement));
  MOCK_METHOD1(how_many_directories_above, unsigned int(const string &statement));
  MOCK_METHOD1(begins_with_dot, bool(const string &statement));
  MOCK_METHOD1(without_prepended_dots, string(const string &statement));
  MOCK_METHOD1(remove_as_statements, string(const string &statement));

  void insert_edge(const string &,
                                const string &,
                                const string &,
                                unsigned int) {}
};

class Mock_Statement_Detector : public Statement_Detector {
 public:
  explicit Mock_Statement_Detector(const Solver::Ptr &solver)
      : Statement_Detector(solver, 0) {}
  optional<pair<string, unsigned int>> call_detect(
      const string &line) const {
    return detect(line);
  }

  void call_process_stream(istream &input, const string &p) {
    process_stream(input, p);
  }
};

TEST_F(SolverPyTest, GetFirstLastSubstring) {

  // Asserts that the first / last substring is returned
  string s = "A string with, some. kind: of: - delimeter";

  EXPECT_THAT("A string with", StrEq(get_first_substring(s,",")));
  EXPECT_THAT("A string", StrNe(get_first_substring(s,",")));
  EXPECT_THAT("A string with, some. kind", StrEq(get_first_substring(s,":")));
  EXPECT_THAT("", StrEq(get_first_substring(s,"^")));  // ^ does not exist in string

  EXPECT_THAT(" delimeter", StrEq(get_final_substring(s,"-")));
  EXPECT_THAT("delimeter", StrNe(get_final_substring(s,"-")));
  EXPECT_THAT(" - delimeter", StrEq(get_final_substring(s,":")));
  EXPECT_THAT("", StrEq(get_final_substring(s,"^"))); // ^ does not exist in string
}

TEST_F(SolverPyTest, DotsToSlash) {
  string input = "string.with.dots";
  path expected{"string"};
  expected /= "with";
  expected /= "dots";
  string output = dots_to_system_slash(input);
  EXPECT_THAT(output, Contains(path::preferred_separator));
  EXPECT_THAT(output, Not(Contains('.')));
  EXPECT_THAT(output, expected);
}


TEST_F(SolverPyTest, DotsToSlash2) {
  string input = "string    .    with.   dots";
  path expected{"string    "};
  expected /= "    with";
  expected /= "   dots";
  string output = dots_to_system_slash(input);
  EXPECT_THAT(output, Contains(path::preferred_separator));
  EXPECT_THAT(output, Not(Contains('.')));
  EXPECT_THAT(output, StrEq(expected.string()));
}

TEST_F(SolverPyTest, ImportToPath) {
  // The "from x import y"-regex returns result in below form
  string input = "foo.bar import baz";
  path expected{"foo"};
  expected /= "bar";
  expected /= "baz";

  string output = convert_import_statement_to_path_str(input);

  EXPECT_THAT(output, Contains(path::preferred_separator));
  EXPECT_THAT(output, Not(Contains('.')));
  EXPECT_THAT(output, Not(Contains(' ')));
  ASSERT_EQ(output.size(), 11);
  EXPECT_THAT(output, StrEq(expected.string()));
}

TEST_F(SolverPyTest, ImportAsToPath) {
  string input = "foo.bar as baz";
  path p{"foo"};
  p /= "bar";
  string output = convert_import_statement_to_path_str(input);

  EXPECT_THAT(output, Contains(path::preferred_separator));
  EXPECT_THAT(output, Not(HasSubstr(".")));
  EXPECT_THAT(output, Not(HasSubstr(" ")));
  EXPECT_THAT(output, p);
}

TEST_F(SolverPyTest, DirectoriesAbove) {
  const string none = " .no.dir.above";
  const string one = " ..one.dir.above";
  const string twenty = " .....................twenty.dirs.above";

  EXPECT_EQ(0, how_many_directories_above(none));
  EXPECT_EQ(1, how_many_directories_above(one));
  EXPECT_EQ(20, how_many_directories_above(twenty));
}

TEST_F(SolverPyTest, StartsWithDot) {
  const string dot_beginning = "    .yes.it.does";
  const string dot_beginning_too = ".dot";
  const string no_dot_beginning = "no.dots";

  EXPECT_TRUE(begins_with_dot(dot_beginning));
  EXPECT_TRUE(begins_with_dot(dot_beginning_too));
  EXPECT_FALSE(begins_with_dot(no_dot_beginning));
}

TEST_F(SolverPyTest, RemoveStartingDots) {
  const string dot_beginning = "....dots";
  const string dot_beginning_too = ".dot";
  const string no_dot_beginning = "no.dots";

  EXPECT_THAT("dots", StrEq(without_prepended_dots(dot_beginning)));
  EXPECT_THAT("dot", StrEq(without_prepended_dots(dot_beginning_too)));
  EXPECT_THAT("no.dots", StrEq(without_prepended_dots(no_dot_beginning)));
}
// NOLINTNEXTLINE
TEST_F(SolverPyTest, emptyinitialization) {
  auto s = make_shared<MockSolver_Py>();
  auto d = make_shared<Statement_Detector>(s);
  EXPECT_EQ(d->get_statements().size(), 2);
  d->wait_for_workers();
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, simpledetection) {
  auto s = make_shared<MockSolver_Py>();
  auto d = make_shared<Mock_Statement_Detector>(s);
  EXPECT_EQ(d->get_statements().size(), 2);
  auto res = d->call_detect("  import xyz");
  EXPECT_EQ(static_cast<bool>(res), true);
  EXPECT_EQ(res->first, "xyz");
  EXPECT_EQ(res->second, 0);
  d->wait_for_workers();
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, NoMatch) {
  auto s = make_shared<MockSolver_Py>();
  auto d = make_shared<Mock_Statement_Detector>(s);

  stringstream sstream;
  sstream << "def fooimport:" << endl;
  sstream << "iomport xxx" << endl;  // invalid statement, should not get
                                          // detected!
  sstream << "xyz" << endl;

  EXPECT_EQ(d->get_statements().size(), 2);
  EXPECT_CALL(*s, add_edge(_, _, _, _)).Times(0);
  d->call_process_stream(sstream, "id");
  d->wait_for_workers();
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, MultiMatch) {
  auto s = make_shared<MockSolver_Py>();
  auto d = make_shared<Mock_Statement_Detector>(s);

  stringstream sstream;
  sstream << "from  abc import xxx" << endl;
  sstream << "import yyy" << endl;

  EXPECT_EQ(d->get_statements().size(), 2);
  EXPECT_CALL(*s, add_edge("id", "abc import xxx", 1, 1));
  EXPECT_CALL(*s, add_edge("id", "yyy", 0, 2));
  d->call_process_stream(sstream, "id");
  d->wait_for_workers();
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, MultiLineMatch) {
  auto s = make_shared<MockSolver_Py>();
  auto d = make_shared<Mock_Statement_Detector>(s);

  string multi_line_string = R"(from yyy\
                             import xxx
                             import\
                             \
                             \
                             \
                             zzz)";
  stringstream sstream(multi_line_string);

  EXPECT_EQ(d->get_statements().size(), 2);
  EXPECT_CALL(*s, add_edge(_, _, _, Ge(2))).Times(2);
  d->call_process_stream(sstream, "id");
  d->wait_for_workers();
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, CommaSeparatedMatch) {
  string statement = "import xxx,yyy";
  EXPECT_THAT(split_comma_string(statement), SizeIs(2));
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, FromXXXImportYYYNoDots) {
  string s = "xxx import yyy";
  EXPECT_THAT("xxx/yyy", StrEq(convert_import_statement_to_path_str(s)));
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, FromXXXImportYYYThreeDots) {
  string s = "...xxx import yyy";
  EXPECT_THAT("../../xxx/yyy", StrEq(convert_import_statement_to_path_str(s)));
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, FromXXXImportYYYAsZZZNoDots) {
  string s = "xxx import yyy as zzz";
  EXPECT_THAT("xxx/yyy", StrEq(convert_import_statement_to_path_str(s)));
}

// NOLINTNEXTLINE
TEST_F(SolverPyTest, FromXXXImportYYYAsZZZThreeDots) {
  string s = "...xxx import yyy as zzz";
  EXPECT_THAT("../../xxx/yyy", StrEq(convert_import_statement_to_path_str(s)));
}*/

// vim: filetype=cpp et ts=2 sw=2 sts=2
