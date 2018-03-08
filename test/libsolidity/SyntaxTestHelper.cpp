/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <test/libsolidity/SyntaxTestHelper.h>
#include <test/libsolidity/AnalysisFramework.h>
#include <test/TestHelper.h>
#include <boost/algorithm/string/replace.hpp>

using namespace std;

namespace dev
{
namespace solidity
{
namespace test
{

void SyntaxTester::runTest(SyntaxTest const& _test)
{
	vector<string> unexpectedErrors;
	auto expectations = _test.expectations;
	auto errorList = parseAnalyseAndReturnError(_test.source, true, true, true).second;

	bool errorsMatch = true;

	if (errorList.size() != expectations.size ())
		errorsMatch = false;
	else
	{
		for (size_t i = 0; i < errorList.size(); i++)
		{
			auto const& error = errorList[i];
			auto const& expectation = expectations[i];
			bool const typeMatches = error->typeName() == expectation.first;
			bool const messageMatches = errorMessage(*error) == expectation.second;
			if (!typeMatches || !messageMatches)
			{
				errorsMatch = false;
				break;
			}
		}
	}

	if (!errorsMatch)
	{
		string msg = "Test expectation mismatch.\nExpected result:\n";
		if (expectations.empty())
			msg += "\tSuccess\n";
		else
			for (auto const &expectation: expectations)
			{
				msg += "\t" + expectation.first + ": " + expectation.second + "\n";
			}
		msg += "Obtained result:\n";
		if (errorList.empty())
			msg += "\tSuccess\n";
		else
			for (auto &error: errorList)
			{
				msg += "\t" + error->typeName() + ": " + errorMessage(*error) + "\n";
			}
		BOOST_ERROR(msg);
	}
}

std::string SyntaxTester::errorMessage(Error const &_e)
{
	if (_e.comment())
	{
		std::string msg = *_e.comment();
		boost::replace_all(msg, "\n", "\\n");
		return msg;
	}
	else
	{
		return "NONE";
	}
}

int SyntaxTester::registerTests(
	boost::unit_test::test_suite& _suite,
	boost::filesystem::path const& _basepath,
	boost::filesystem::path const& _path
)
{

	int numTestsAdded = 0;
	boost::filesystem::path fullpath = _basepath / _path;
	if (boost::filesystem::is_directory(fullpath))
	{
		boost::unit_test::test_suite* sub_suite = BOOST_TEST_SUITE(_path.filename().string());
		for (auto const& entry: boost::filesystem::directory_iterator(fullpath))
			numTestsAdded += registerTests(*sub_suite, _basepath, _path / entry.path().filename());
		_suite.add(sub_suite);
	}
	else
	{
		_suite.add(boost::unit_test::make_test_case(
			[fullpath] { SyntaxTester().runTest(SyntaxTestParser().parse(fullpath.string())); },
			_path.stem().string(),
			_path.string(),
			0
		));
		numTestsAdded = 1;
	}
	return numTestsAdded;
}

void SyntaxTester::registerTests()
{
	if(dev::test::Options::get().testPath.empty())
		throw runtime_error(
			"No path to the test files was specified. "
			"Use the --testpath command line option or "
			"the ETH_TEST_PATH environment variable."
		);
	auto testPath = boost::filesystem::path(dev::test::Options::get().testPath);

	if (boost::filesystem::exists(testPath) && boost::filesystem::is_directory(testPath))
	{
		int numTestsAdded = registerTests(
			boost::unit_test::framework::master_test_suite(),
			testPath / "libsolidity",
			"syntaxTests"
		);
		solAssert(numTestsAdded > 0, "no syntax tests found in libsolidity/syntaxTests");
	}
	else
		solAssert(false, "libsolidity/syntaxTests directory not found");
}

}
}
}