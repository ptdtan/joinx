#include "fileformats/vcf/AltNormalizer.hpp"
#include "fileformats/vcf/Entry.hpp"
#include "fileformats/vcf/Header.hpp"
#include "fileformats/Fasta.hpp"
#include "fileformats/InputStream.hpp"

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace Vcf;
using namespace std;

class TestVcfAltNormalizer : public ::testing::Test {
protected:
    TestVcfAltNormalizer()
        : _refFa(">1\nTTTCGCGCGCGCG")
        , _ref("test", _refFa.data(), _refFa.size())
    {
    }

    void SetUp() {
        stringstream hdrss(
            "##fileformat=VCFv4.1\n"
            "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tS1\tS2\n"
            );

        InputStream in("test", hdrss);
        _header = Header::fromStream(in);
    }

    Entry makeEntry(string chrom, int64_t pos, string const& ref, string const& alt, string const& sampleData = ".\t.") {
        stringstream ss;
        ss << chrom << "\t" << pos << "\t.\t" << ref << "\t" << alt << "\t.\t.\t.\tGT\t" << sampleData;
        return Entry(&_header, ss.str());
    }

    string _refFa;
    Fasta _ref;
    Header _header;
};

TEST_F(TestVcfAltNormalizer, equivalentAlts) {
    string ref = _ref.sequence("1", 4, 6);
    ASSERT_EQ("CGCGCG", ref);
    Entry e = makeEntry("1", 4, ref, "CGCGCG,CGCG", "0/1\t1/2");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(3u, e.pos());
    ASSERT_EQ("TCG", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("T", e.alt()[0]);
    ASSERT_EQ("0/0", e.sampleData().genotype(0).string());
    ASSERT_EQ("0/1", e.sampleData().genotype(1).string());
}

// single alt cases
TEST_F(TestVcfAltNormalizer, insertion) {
    cout << "   REF: " << _ref.sequence("1", 1, 13) << "\n";
    string ref = _ref.sequence("1", 11, 3);
    ASSERT_EQ("GCG", ref);
    Entry e = makeEntry("1", 11, ref, "GCGCG");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(3u, e.pos());
    ASSERT_EQ("T", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("TCG", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, insertionWithTrailingRepeatMatch) {
    string ref = _ref.sequence("1", 11, 3);
    ASSERT_EQ("GCG", ref);
    Entry e = makeEntry("1", 11, ref, "GAGCG");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(10u, e.pos());
    ASSERT_EQ("C", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("CGA", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, immovableInsertion) {
    string ref = _ref.sequence("1", 11, 3);
    ASSERT_EQ("GCG", ref);
    Entry e = makeEntry("1", 11, ref, "GAATT");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(12u, e.pos());
    ASSERT_EQ("CG", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("AATT", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, deletion) {
    string ref = _ref.sequence("1", 11, 3);
    ASSERT_EQ("GCG", ref);
    Entry e = makeEntry("1", 11, ref, "G");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(3u, e.pos());
    ASSERT_EQ("TCG", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("T", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, deletionWithSubstitution) {
    string ref = _ref.sequence("1", 9, 5);
    ASSERT_EQ("GCGCG", ref);
    Entry e = makeEntry("1", 9, ref, "GAG");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(10u, e.pos());
    ASSERT_EQ("CGC", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("A", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, immovableDeletion) {
    string ref = _ref.sequence("1", 9, 5);
    ASSERT_EQ("GCGCG", ref);
    Entry e = makeEntry("1", 9, ref, "GAT");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(10u, e.pos());
    ASSERT_EQ("CGCG", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("AT", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, testSubstitution) {
    string ref = _ref.sequence("1", 9, 1);
    ASSERT_EQ("G", ref);
    Entry e = makeEntry("1", 9, ref, "C");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(9u, e.pos());
    ASSERT_EQ("G", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("C", e.alt()[0]);
}

TEST_F(TestVcfAltNormalizer, testSubstitutionWithPadding) {
    string ref = _ref.sequence("1", 9, 5);
    ASSERT_EQ("GCGCG", ref);
    Entry e = makeEntry("1", 9, ref, "GCGCA");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";

    ASSERT_EQ(13u, e.pos());
    ASSERT_EQ("G", e.ref());
    ASSERT_EQ(1u, e.alt().size());
    ASSERT_EQ("A", e.alt()[0]);
}

// multi alt cases
TEST_F(TestVcfAltNormalizer, insertionAndDeletion) {
    string ref = _ref.sequence("1", 11, 3);
    ASSERT_EQ("GCG", ref);
    Entry e = makeEntry("1", 11, ref, "GCGCG,GC");
    AltNormalizer n(_ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";
}

TEST_F(TestVcfAltNormalizer, messyInsertionAndDeletion) {
    string refStr(">1\nTTTTTTTTTTTTTCCTCGCTCCC");
    Fasta ref("test", refStr.data(), refStr.size());
    Entry e = makeEntry("1", 22, "CC",  "C,CCTCGCTCCC");
    AltNormalizer n(ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";
    ASSERT_EQ(13u, e.pos());
    ASSERT_EQ("TCCTCGCTC", e.ref());
    ASSERT_EQ("TCCTCGCT", e.alt()[0]);
    ASSERT_EQ("TCCTCGCTCCCTCGCTC", e.alt()[1]);
}

TEST_F(TestVcfAltNormalizer, indelAtPos1) {
    string refStr(">1\nAGAGAGAAAGAAAG");
    Fasta ref("test", refStr.data(), refStr.size());
    Entry e = makeEntry("1", 2, "GAG",  "G");
    AltNormalizer n(ref);
    cout << "BEFORE: " << e << "\n";
    n.normalize(e);
    cout << " AFTER: " << e << "\n";
    ASSERT_EQ(1u, e.pos());
    ASSERT_EQ("AGA", e.ref());
    ASSERT_EQ("A", e.alt()[0]);

}
