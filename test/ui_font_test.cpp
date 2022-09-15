
#include "tests.hpp"
#include <boost/test/unit_test.hpp>
#include <lvtk/ui/font.hpp>

BOOST_AUTO_TEST_SUITE (Font)

BOOST_AUTO_TEST_CASE (styles) {
    using Font      = lvtk::Font;
    using FontStyle = lvtk::FontStyle;

    auto f = Font();
    BOOST_REQUIRE_EQUAL (f.normal(), true);
    BOOST_REQUIRE_EQUAL (f.bold(), false);
    BOOST_REQUIRE_EQUAL (f.italic(), false);
    BOOST_REQUIRE_EQUAL (f.underline(), false);

    f = f.with_style (FontStyle::BOLD);
    BOOST_REQUIRE_EQUAL (f.normal(), false);
    BOOST_REQUIRE_EQUAL (f.bold(), true);
    BOOST_REQUIRE_EQUAL (f.italic(), false);
    BOOST_REQUIRE_EQUAL (f.underline(), false);

    f = f.with_style (f.flags() | FontStyle::UNDERLINE);
    BOOST_REQUIRE_EQUAL (f.normal(), false);
    BOOST_REQUIRE_EQUAL (f.bold(), true);
    BOOST_REQUIRE_EQUAL (f.italic(), false);
    BOOST_REQUIRE_EQUAL (f.underline(), true);

    f = f.with_style (f.flags() | FontStyle::ITALIC);
    BOOST_REQUIRE_EQUAL (f.normal(), false);
    BOOST_REQUIRE_EQUAL (f.bold(), true);
    BOOST_REQUIRE_EQUAL (f.italic(), true);
    BOOST_REQUIRE_EQUAL (f.underline(), true);

    auto f2 = Font (f.height(), FontStyle::BOLD | FontStyle::ITALIC | FontStyle::UNDERLINE);
    auto fonts_equal = f == f2;
    BOOST_REQUIRE_EQUAL(fonts_equal, true);
    
    f2 = f2.with_height(f2.height() - 1.f);
    auto fonts_not_equal = f != f2;
    BOOST_REQUIRE_EQUAL (fonts_not_equal, true);
}

BOOST_AUTO_TEST_SUITE_END()
