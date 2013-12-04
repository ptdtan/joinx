#include "fileformats/vcf/GenotypeComparator.hpp"
#include "fileformats/vcf/Entry.hpp"
#include "fileformats/vcf/RawVariant.hpp"
#include "common/cstdint.hpp"
#include "io/StreamJoin.hpp"

#include <gtest/gtest.h>
#include <boost/ref.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    size_t nStreams = 3;

    struct Collector {
        void operator()(
                size_t sampleIdx,
                std::string const& sequence,
                Vcf::RawVariant::Vector const& vars,
                std::set<size_t> const& which)
        {
            std::map<std::string, std::vector<std::string>> calls;

            for (auto i = vars.begin(); i != vars.end(); ++i) {
                std::stringstream refpos;
                refpos << i->pos << ": " << i->ref;
                calls[refpos.str()].push_back(i->alt);
            }

            std::vector<std::string> xs;
            for (auto i = calls.begin(); i != calls.end(); ++i) {
                std::stringstream ss;
                ss << i->first << " -> "
                    << streamJoin(i->second).delimiter("/").emptyString(".")
                    ;

                xs.push_back(ss.str());
            }

            std::cout << sampleIdx << ": "
                << sequence << "\t"
                << streamJoin(xs).delimiter(",") << "\t"
                << streamJoin(which).delimiter(",") << "\n";
        }
    };
}

class TestVcfGenotypeComparator : public ::testing::Test {
public:
    void SetUp() {
        std::string header(
            "##fileformat=VCFv4.1\n"
            "##FORMAT=<ID=GT,Type=String,Number=1,Description=\"Genotype\">\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tS1\tS2\n"
            );

        sampleNames_.push_back("S1");
        sampleNames_.push_back("S2");

        for (size_t i = 0; i < nStreams; ++i) {
            std::stringstream ss(header);
            // we happen to need pointers, so copy to heap
            headers_.push_back(new Vcf::Header(Vcf::Header::fromStream(ss)));
            headers_.back()->sourceIndex(i);
        }
    }

    void TearDown() {
        for (auto x = headers_.begin(); x != headers_.end(); ++x) {
            delete *x;
        }
    }


    Vcf::Entry* makeEntry(
            size_t headerIdx,
            std::string chrom,
            int64_t pos,
            std::string const& ref,
            std::string const& alt,
            std::string const& gt = "."
            )
    {
        stringstream ss;
        ss << chrom << "\t" << pos << "\t.\t" << ref << "\t" << alt << "\t.\t.\t.\tGT\t" << gt;
        return new Vcf::Entry(headers_[headerIdx], ss.str());
    }

protected:
    std::vector<std::string> sampleNames_;
    std::vector<Vcf::Header*> headers_;
};

TEST_F(TestVcfGenotypeComparator, process) {
    using namespace Vcf;
    Collector c;
    auto gcmp = makeGenotypeComparator(sampleNames_, headers_, nStreams, c);
    gcmp.push(makeEntry(0, "1", 10, "A", "G", "1/1\t0/1"));
    gcmp.push(makeEntry(1, "1", 10, "A", "G", "0/1\t1/1"));
    gcmp.push(makeEntry(2, "1", 10, "A", "C", "0/1"));
    gcmp.push(makeEntry(2, "1", 11, "A", "G", "0/1"));
    gcmp.push(makeEntry(0, "1", 11, "A", "G", "0/1"));
    gcmp.push(makeEntry(0, "1", 12, "A", "G,C", "1/1"));
    gcmp.push(makeEntry(0, "1", 12, "AA", "A,C", "1/1\t1/2"));

    gcmp.finalize();
}
