// Copyright 2022 Michael Fisher <mfisher@lvtk.org>
// SPDX-License-Identifier: ISC

#include "lvtk/ext/bufsize.hpp"
#include "lvtk/ext/options.hpp"
#include "lvtk/lvtk.hpp"
#include "lvtk/optional.hpp"
#include "lvtk/options.hpp"
#include "lvtk/plugin.hpp"
#include "lvtk/symbols.hpp"

#include <boost/test/unit_test.hpp>

#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/options/options.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

struct BufSizePlug : lvtk::Plugin<BufSizePlug, lvtk::BufSize> {
    BufSizePlug (const lvtk::Args& args) : Plugin (args) {}
};

class BufSizeTest {
public:
    BufSizeTest() {
        details.min = 64;
        details.max = 4096;
        details.nominal = 128;
        details.sequence_size = 1024 * 8;

        subject = urid.map ("http://dummy.subject.org");
        type = urid.map ("http://www.w3.org/2001/XMLSchema#nonNegativeInteger");
        try {
            options.add (
                       LV2_OPTIONS_BLANK, subject, urid.map (LV2_BUF_SIZE__minBlockLength), sizeof (uint32_t), type, &*details.min)
                .add (
                    LV2_OPTIONS_BLANK, subject, urid.map (LV2_BUF_SIZE__maxBlockLength), sizeof (uint32_t), type, &*details.max)
                // leave commented: check not set by mixin
                // .add (
                //     LV2_OPTIONS_BLANK, subject,
                //     urid.map (LV2_BUF_SIZE__nominalBlockLength),
                //     sizeof(uint32_t), type,
                //     &*details.nominal
                // )
                .add (
                    LV2_OPTIONS_BLANK, subject, urid.map (LV2_BUF_SIZE__sequenceSize), sizeof (uint32_t), type, &*details.sequence_size);
        } catch (std::exception& e) {
            BOOST_ERROR (e.what());
        }

        args.sample_rate = 44100.0;
        args.bundle = "/fake/bundle.lv2";
        options_feature.URI = LV2_OPTIONS__options;
        options_feature.data = const_cast<lvtk::Option*> (options.get());
        args.features.push_back (options_feature);
        args.features.push_back (*urid.get_map_feature());
    }

    void buffer_details() {
        auto plugin = std::unique_ptr<BufSizePlug> (new BufSizePlug (args));
        const auto& pdetails = plugin->buffer_details();

        BOOST_ASSERT ((bool) pdetails.min);
        BOOST_ASSERT (details.min == pdetails.min);
        BOOST_ASSERT ((bool) pdetails.max);
        BOOST_ASSERT (details.max == pdetails.max);

        BOOST_ASSERT (! (bool) pdetails.nominal);
        BOOST_ASSERT (details.nominal != pdetails.nominal);
    }

private:
    lvtk::Args args;
    LV2_Feature options_feature = { LV2_OPTIONS__options, nullptr };
    lvtk::BufferDetails details;
    lvtk::OptionArray options;
    lvtk::URIDirectory urid;
    uint32_t subject;
    uint32_t type;
};

using namespace lvtk;

BOOST_AUTO_TEST_SUITE (BufSize)

BOOST_AUTO_TEST_CASE (buffer_details) {
    BufSizeTest().buffer_details();
}

BOOST_AUTO_TEST_SUITE_END()
