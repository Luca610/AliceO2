// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Framework/ASoA.h"
#include "Framework/TableBuilder.h"
#include "Framework/GroupSlicer.h"
#include "Framework/ArrowTableSlicingCache.h"
#include <arrow/util/config.h>

#include <catch_amalgamated.hpp>

using namespace o2;
using namespace o2::framework;

namespace o2::aod
{
namespace test
{
DECLARE_SOA_COLUMN(ID, id, int);
DECLARE_SOA_COLUMN(Foo, foo, float);
DECLARE_SOA_COLUMN(Bar, bar, float);
DECLARE_SOA_COLUMN(EventProperty, eventProperty, float);
} // namespace test
DECLARE_SOA_TABLE(Events, "AOD", "EVTS",
                  o2::soa::Index<>,
                  test::ID,
                  test::EventProperty,
                  test::Foo,
                  test::Bar);

using Event = Events::iterator;

namespace test
{
DECLARE_SOA_INDEX_COLUMN(Event, event);
DECLARE_SOA_COLUMN(X, x, float);
DECLARE_SOA_COLUMN(Y, y, float);
DECLARE_SOA_COLUMN(Z, z, float);
} // namespace test

namespace unsorted
{
DECLARE_SOA_INDEX_COLUMN(Event, event);
}

DECLARE_SOA_TABLE(TrksX, "AOD", "TRKSX",
                  test::EventId,
                  test::X);
DECLARE_SOA_TABLE(TrksY, "AOD", "TRKSY",
                  test::EventId,
                  test::Y);
DECLARE_SOA_TABLE(TrksZ, "AOD", "TRKSZ",
                  test::EventId,
                  test::Z);
DECLARE_SOA_TABLE(TrksU, "AOD", "TRKSU",
                  test::X,
                  test::Y,
                  test::Z);

DECLARE_SOA_TABLE(TrksXU, "AOD", "TRKSX",
                  unsorted::EventId,
                  test::X);
DECLARE_SOA_TABLE(TrksYU, "AOD", "TRKSY",
                  unsorted::EventId,
                  test::Y);
DECLARE_SOA_TABLE(TrksZU, "AOD", "TRKSZ",
                  unsorted::EventId,
                  test::Z);

namespace test
{
DECLARE_SOA_COLUMN(Arr, arr, float[3]);
DECLARE_SOA_COLUMN(Boo, boo, bool);
DECLARE_SOA_COLUMN(Lst, lst, std::vector<double>);
} // namespace test

DECLARE_SOA_TABLE(EventExtra, "AOD", "EVTSXTRA", test::Arr, test::Boo, test::Lst);

} // namespace o2::aod
TEST_CASE("GroupSlicerOneAssociated")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksX>();
  for (auto i = 0; i < 20; ++i) {
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriter(0, i, 0.5f * j);
    }
  }
  auto trkTable = builderT.finalize();
  aod::Events e{evtTable};
  aod::TrksX t{trkTable};
  REQUIRE(e.size() == 20);
  REQUIRE(t.size() == 10 * 20);

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntry(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  auto count = 0;
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == count);
    auto trks = std::get<aod::TrksX>(as);
    REQUIRE(trks.size() == 10);
    for (auto& trk : trks) {
      REQUIRE(trk.eventId() == count);
    }
    ++count;
  }
}

TEST_CASE("GroupSlicerSeveralAssociated")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderTX;
  auto trksWriterX = builderTX.cursor<aod::TrksX>();
  TableBuilder builderTY;
  auto trksWriterY = builderTY.cursor<aod::TrksY>();
  TableBuilder builderTZ;
  auto trksWriterZ = builderTZ.cursor<aod::TrksZ>();

  TableBuilder builderTXYZ;
  auto trksWriterXYZ = builderTXYZ.cursor<aod::TrksU>();

  for (auto i = 0; i < 20; ++i) {
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriterX(0, i, 0.5f * j);
    }
    for (auto j = 0.f; j < 10; j += 0.5f) {
      trksWriterY(0, i, 0.5f * j);
    }
    for (auto j = 0.f; j < 15; j += 0.5f) {
      trksWriterZ(0, i, 0.5f * j);
    }

    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriterXYZ(0, 0.5f * j, 2.f * j, 2.5f * j);
    }
  }
  auto trkTableX = builderTX.finalize();
  auto trkTableY = builderTY.finalize();
  auto trkTableZ = builderTZ.finalize();

  auto trkTableXYZ = builderTXYZ.finalize();

  aod::Events e{evtTable};
  aod::TrksX tx{trkTableX};
  aod::TrksY ty{trkTableY};
  aod::TrksZ tz{trkTableZ};

  aod::TrksU tu{trkTableXYZ};

  REQUIRE(e.size() == 20);
  REQUIRE(tx.size() == 10 * 20);
  REQUIRE(ty.size() == 20 * 20);
  REQUIRE(tz.size() == 30 * 20);

  REQUIRE(tu.size() == 10 * 20);

  auto tt = std::make_tuple(tx, ty, tz, tu);
  auto key = "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>());
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), key},
                                 {soa::getLabelFromType<aod::TrksY>(), key},
                                 {soa::getLabelFromType<aod::TrksZ>(), key}});
  auto s = slices.updateCacheEntry(0, {trkTableX});
  s = slices.updateCacheEntry(1, {trkTableY});
  s = slices.updateCacheEntry(2, {trkTableZ});
  o2::framework::GroupSlicer g(e, tt, slices);

  auto count = 0;
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == count);
    auto trksx = std::get<aod::TrksX>(as);
    auto trksy = std::get<aod::TrksY>(as);
    auto trksz = std::get<aod::TrksZ>(as);

    auto trksu = std::get<aod::TrksU>(as);

    REQUIRE(trksx.size() == 10);
    REQUIRE(trksy.size() == 20);
    REQUIRE(trksz.size() == 30);

    REQUIRE(trksu.size() == 10 * 20);

    for (auto& trk : trksx) {
      REQUIRE(trk.eventId() == count);
    }
    for (auto& trk : trksy) {
      REQUIRE(trk.eventId() == count);
    }
    for (auto& trk : trksz) {
      REQUIRE(trk.eventId() == count);
    }

    ++count;
  }
}

TEST_CASE("GroupSlicerMismatchedGroups")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksX>();
  for (auto i = 0; i < 20; ++i) {
    if (i == 3 || i == 10 || i == 12 || i == 16 || i == 19) {
      continue;
    }
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriter(0, i, 0.5f * j);
    }
  }
  auto trkTable = builderT.finalize();
  aod::Events e{evtTable};
  aod::TrksX t{trkTable};
  REQUIRE(e.size() == 20);
  REQUIRE(t.size() == 10 * (20 - 5));

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntry(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  auto count = 0;
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == count);
    auto trks = std::get<aod::TrksX>(as);
    if (count == 3 || count == 10 || count == 12 || count == 16 || count == 19) {
      REQUIRE(trks.size() == 0);
    } else {
      REQUIRE(trks.size() == 10);
    }
    for (auto& trk : trks) {
      REQUIRE(trk.eventId() == count);
    }
    ++count;
  }
}

TEST_CASE("GroupSlicerMismatchedUnassignedGroups")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  int skip = 0;

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksX>();
  for (auto i = 0; i < 20; ++i) {
    if (i == 3 || i == 10 || i == 12 || i == 16 || i == 19) {
      for (auto iz = 0; iz < 5; ++iz) {
        trksWriter(0, -1 - skip, 0.123f * iz + 1.654f);
      }
      ++skip;
      continue;
    }
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriter(0, i, 0.5f * j);
    }
  }
  for (auto i = 0; i < 5; ++i) {
    trksWriter(0, -6, 0.123f * i + 1.654f);
  }
  auto trkTable = builderT.finalize();

  aod::Events e{evtTable};
  aod::TrksX t{trkTable};
  REQUIRE(e.size() == 20);
  REQUIRE(t.size() == (30 + 10 * (20 - 5)));

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntry(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  auto count = 0;
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == count);
    auto trks = std::get<aod::TrksX>(as);
    if (count == 3 || count == 10 || count == 12 || count == 16 || count == 19) {
      REQUIRE(trks.size() == 0);
    } else {
      REQUIRE(trks.size() == 10);
    }
    for (auto& trk : trks) {
      REQUIRE(trk.eventId() == count);
    }
    ++count;
  }
}

TEST_CASE("GroupSlicerMismatchedFilteredGroups")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksX>();
  for (auto i = 0; i < 20; ++i) {
    if (i == 3 || i == 10 || i == 12 || i == 16) {
      continue;
    }
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriter(0, i, 0.5f * j);
    }
  }
  auto trkTable = builderT.finalize();
  using FilteredEvents = soa::Filtered<aod::Events>;
  soa::SelectionVector rows{2, 4, 10, 9, 15};
  FilteredEvents e{{evtTable}, {2, 4, 10, 9, 15}};
  aod::TrksX t{trkTable};
  REQUIRE(e.size() == 5);
  REQUIRE(t.size() == 10 * (20 - 4));

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntry(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  auto count = 0;

  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == rows[count]);
    auto trks = std::get<aod::TrksX>(as);
    if (rows[count] == 3 || rows[count] == 10 || rows[count] == 12 || rows[count] == 16) {
      REQUIRE(trks.size() == 0);
    } else {
      REQUIRE(trks.size() == 10);
    }
    for (auto& trk : trks) {
      REQUIRE(trk.eventId() == rows[count]);
    }
    ++count;
  }
}

TEST_CASE("GroupSlicerMismatchedUnsortedFilteredGroups")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksXU>();
  std::vector<int> randomized{10, 2, 1, 0, 15, 3, 6, 4, 14, 5, 7, 9, 8, 19, 11, 13, 17, 12, 18, 19};
  std::vector<int64_t> sel;
  sel.resize(10 * (20 - 4));
  std::iota(sel.begin(), sel.end(), 0);
  for (auto i : randomized) {
    if (i == 3 || i == 10 || i == 12 || i == 16) {
      continue;
    }
    for (auto j = 0.f; j < 5; j += 0.5f) {
      trksWriter(0, i, 0.5f * j);
    }
  }
  auto trkTable = builderT.finalize();

  TableBuilder builderTE;
  auto trksWriterE = builderTE.cursor<aod::TrksXU>();
  auto trkTableE = builderTE.finalize();

  using FilteredEvents = soa::Filtered<aod::Events>;
  soa::SelectionVector rows{2, 4, 10, 9, 15};
  FilteredEvents e{{evtTable}, {2, 4, 10, 9, 15}};
  soa::SmallGroups<aod::TrksXU> t{{trkTable}, std::move(sel)};

  REQUIRE(e.size() == 5);
  REQUIRE(t.size() == 10 * (20 - 4));

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({}, {{soa::getLabelFromType<aod::TrksXU>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntryUnsorted(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  unsigned int count = 0;

  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == rows[count]);
    auto trks = std::get<soa::SmallGroups<aod::TrksXU>>(as);
    if (rows[count] == 3 || rows[count] == 10 || rows[count] == 12 || rows[count] == 16) {
      REQUIRE(trks.size() == 0);
    } else {
      REQUIRE(trks.size() == 10);
    }
    for (auto& trk : trks) {
      REQUIRE(trk.eventId() == rows[count]);
    }
    ++count;
  }

  std::vector<int64_t> sele;
  soa::SmallGroups<aod::TrksXU> te{{trkTableE}, std::move(sele)};
  auto tte = std::make_tuple(te);
  o2::framework::GroupSlicer ge(e, tte, slices);

  count = 0;
  for (auto& slice : ge) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == rows[count]);
    auto trks = std::get<soa::SmallGroups<aod::TrksXU>>(as);
    REQUIRE(trks.size() == 0);
    ++count;
  }

  soa::SmallGroupsUnfiltered<aod::TrksXU> tu{{trkTable}, std::vector<int64_t>{}};
  auto ttu = std::make_tuple(tu);
  o2::framework::GroupSlicer gu(e, ttu, slices);

  count = 0;
  for (auto& slice : gu) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    REQUIRE(gg.globalIndex() == rows[count]);
    auto trks = std::get<soa::SmallGroupsUnfiltered<aod::TrksXU>>(as);
    if (rows[count] == 3 || rows[count] == 10 || rows[count] == 12 || rows[count] == 16) {
      REQUIRE(trks.size() == 0);
    } else {
      REQUIRE(trks.size() == 10);
    }
    ++count;
  }
}

namespace o2::aod
{
namespace parts
{
DECLARE_SOA_INDEX_COLUMN(Event, event);
DECLARE_SOA_COLUMN(Property, property, int);
DECLARE_SOA_SELF_SLICE_INDEX_COLUMN(Relatives, relatives);
} // namespace parts
DECLARE_SOA_TABLE(Parts, "AOD", "PRTS", soa::Index<>, parts::EventId, parts::Property, parts::RelativesIdSlice);

namespace things
{
DECLARE_SOA_INDEX_COLUMN(Event, event);
DECLARE_SOA_INDEX_COLUMN(Part, part);
} // namespace things
DECLARE_SOA_TABLE(Things, "AOD", "THNGS", soa::Index<>, things::EventId, things::PartId);

} // namespace o2::aod

template <typename... As>
static void overwriteInternalIndices(std::tuple<As...>& dest, std::tuple<As...> const& src)
{
  (std::get<As>(dest).bindInternalIndicesTo(&std::get<As>(src)), ...);
}

TEST_CASE("GroupSlicerMismatchedUnsortedFilteredGroupsWithSelfIndex")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderP;
  auto partsWriter = builderP.cursor<aod::Parts>();
  int filler[2];
  std::random_device rd;  // a seed source for the random number engine
  std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> distrib(0, 99);

  for (auto i = 0; i < 100; ++i) {

    filler[0] = distrib(gen);
    filler[1] = distrib(gen);
    if (filler[0] > filler[1]) {
      std::swap(filler[0], filler[1]);
    }
    partsWriter(0, std::floor(i / 10.), i, filler);
  }
  auto partsTable = builderP.finalize();

  TableBuilder builderT;
  auto thingsWriter = builderT.cursor<aod::Things>();
  for (auto i = 0; i < 10; ++i) {
    thingsWriter(0, i, distrib(gen));
  }
  auto thingsTable = builderT.finalize();

  aod::Events e{evtTable};
  // aod::Parts p{partsTable};
  aod::Things t{thingsTable};
  using FilteredParts = soa::Filtered<aod::Parts>;
  auto size = distrib(gen);
  soa::SelectionVector rows;
  for (auto i = 0; i < size; ++i) {
    rows.push_back(distrib(gen));
  }
  FilteredParts fp{{partsTable}, rows};
  auto associatedTuple = std::make_tuple(fp, t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::Parts>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())},
                                 {soa::getLabelFromType<aod::Things>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s0 = slices.updateCacheEntry(0, partsTable);
  auto s1 = slices.updateCacheEntry(1, thingsTable);
  o2::framework::GroupSlicer g(e, associatedTuple, slices);

  overwriteInternalIndices(associatedTuple, associatedTuple);

  // For a grouped case, the recursive access of a slice-self index of a filtered table should have consistent types
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    overwriteInternalIndices(as, associatedTuple);
    auto& ts = std::get<1>(as);
    ts.bindExternalIndices(&e, &std::get<0>(associatedTuple));
    for (auto& thing : ts) {
      if (thing.has_part()) {
        auto part = thing.part_as<FilteredParts>();
        REQUIRE(std::is_same_v<std::decay_t<decltype(part)>::parent_t, FilteredParts>);
        auto rs = part.relatives_as<std::decay_t<decltype(part)::parent_t>>();
        REQUIRE(std::is_same_v<std::decay_t<decltype(rs)>, FilteredParts>);
        for (auto& r : rs) {
          REQUIRE(std::is_same_v<std::decay_t<decltype(r)>::parent_t, FilteredParts>);
          auto rss = r.relatives_as<std::decay_t<decltype(r)>::parent_t>();
          REQUIRE(std::is_same_v<std::decay_t<decltype(rss)>, FilteredParts>);
          for (auto& rr : rss) {
            REQUIRE(std::is_same_v<std::decay_t<decltype(rr)>::parent_t, FilteredParts>);
            auto rsss = rr.relatives_as<std::decay_t<decltype(rr)>::parent_t>();
            REQUIRE(std::is_same_v<std::decay_t<decltype(rsss)>, FilteredParts>);
            for (auto& rrr : rsss) {
              REQUIRE(std::is_same_v<std::decay_t<decltype(rrr)>::parent_t, FilteredParts>);
              auto rssss = rrr.relatives_as<std::decay_t<decltype(rrr)>::parent_t>();
              REQUIRE(std::is_same_v<std::decay_t<decltype(rssss)>, FilteredParts>);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("EmptySliceables")
{
  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  for (auto i = 0; i < 20; ++i) {
    evtsWriter(0, i, 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderT;
  auto trksWriter = builderT.cursor<aod::TrksX>();
  auto trkTable = builderT.finalize();

  aod::Events e{evtTable};
  aod::TrksX t{trkTable};
  REQUIRE(e.size() == 20);
  REQUIRE(t.size() == 0);

  auto tt = std::make_tuple(t);
  ArrowTableSlicingCache slices({{soa::getLabelFromType<aod::TrksX>(), "fIndex" + o2::framework::cutString(soa::getLabelFromType<aod::Events>())}});
  auto s = slices.updateCacheEntry(0, trkTable);
  o2::framework::GroupSlicer g(e, tt, slices);

  unsigned int count = 0;
  for (auto& slice : g) {
    auto as = slice.associatedTables();
    auto gg = slice.groupingElement();
    auto trks = std::get<aod::TrksX>(as);
    REQUIRE(gg.globalIndex() == count);
    REQUIRE(trks.size() == 0);
    ++count;
  }
}

TEST_CASE("ArrowDirectSlicing")
{
  int counts[] = {5, 5, 5, 4, 1};
  int offsets[] = {0, 5, 10, 15, 19, 20};
  int ids[] = {0, 1, 2, 3, 4};
  int sizes[] = {4, 1, 12, 5, 2};

  using BigE = soa::Join<aod::Events, aod::EventExtra>;

  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  auto step = 0;
  for (auto i = 0; i < 20; ++i) {
    if (i >= offsets[step + 1]) {
      ++step;
    }
    evtsWriter(0, ids[step], 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  TableBuilder builderEE;
  auto evtsEWriter = builderEE.cursor<aod::EventExtra>();
  step = 0;

  for (auto i = 0; i < 20; ++i) {
    if (i >= offsets[step + 1]) {
      ++step;
    }
    float arr[3] = {0.1f * i, 0.2f * i, 0.3f * i};
    std::vector<double> d;
    for (auto z = 0; z < sizes[step]; ++z) {
      d.push_back((double)z * 0.5);
    }
    evtsEWriter(0, arr, i % 2 == 0, d);
  }
  auto evtETable = builderEE.finalize();

  aod::Events e{evtTable};
  aod::EventExtra ee{evtETable};
  BigE b_e{{evtTable, evtETable}};

  std::vector<std::shared_ptr<arrow::ChunkedArray>> slices_array;
  std::vector<std::shared_ptr<arrow::ChunkedArray>> slices_bool;
  std::vector<std::shared_ptr<arrow::ChunkedArray>> slices_vec;
  auto offset = 0;
  for (auto i = 0u; i < 5; ++i) {
    slices_array.emplace_back(evtETable->column(0)->Slice(offset, counts[i]));
    slices_bool.emplace_back(evtETable->column(1)->Slice(offset, counts[i]));
    slices_vec.emplace_back(evtETable->column(2)->Slice(offset, counts[i]));
    offset += counts[i];
    REQUIRE(slices_array[i]->length() == counts[i]);
    REQUIRE(slices_bool[i]->length() == counts[i]);
    REQUIRE(slices_vec[i]->length() == counts[i]);
  }

  std::vector<arrow::Datum> slices;
  std::vector<uint64_t> offsts;
  auto bk = std::make_pair(soa::getLabelFromType<aod::Events>(), "fID");
  ArrowTableSlicingCache cache({bk});
  auto s = cache.updateCacheEntry(0, {evtTable});
  auto lcache = cache.getCacheFor(bk);
  for (auto i = 0u; i < 5; ++i) {
    auto [offset, count] = lcache.getSliceFor(i);
    auto tbl = b_e.asArrowTable()->Slice(offset, count);
    auto ca = tbl->GetColumnByName("fArr");
    auto cb = tbl->GetColumnByName("fBoo");
    auto cv = tbl->GetColumnByName("fLst");
    REQUIRE(ca->length() == counts[i]);
    REQUIRE(cb->length() == counts[i]);
    REQUIRE(cv->length() == counts[i]);
    REQUIRE(ca->Equals(slices_array[i]));
    REQUIRE(cb->Equals(slices_bool[i]));
    REQUIRE(cv->Equals(slices_vec[i]));
  }

  int j = 0u;
  for (auto i = 0u; i < 5; ++i) {
    auto [offset, count] = lcache.getSliceFor(i);
    auto tbl = BigE{{b_e.asArrowTable()->Slice(offset, count)}, static_cast<uint64_t>(offset)};
    REQUIRE(tbl.size() == counts[i]);
    for (auto& row : tbl) {
      REQUIRE(row.id() == ids[i]);
      REQUIRE(row.boo() == (j % 2 == 0));
      auto rid = row.globalIndex();
      auto arr = row.arr();
      REQUIRE(arr[0] == 0.1f * (float)rid);
      REQUIRE(arr[1] == 0.2f * (float)rid);
      REQUIRE(arr[2] == 0.3f * (float)rid);

      auto d = row.lst();
      REQUIRE(d.size() == (size_t)sizes[i]);
      for (auto z = 0u; z < d.size(); ++z) {
        REQUIRE(d[z] == 0.5 * (double)z);
      }
      ++j;
    }
  }
}

TEST_CASE("TestSlicingException")
{
  int offsets[] = {0, 5, 10, 15, 19, 20};
  int ids[] = {0, 1, 2, 4, 3};

  TableBuilder builderE;
  auto evtsWriter = builderE.cursor<aod::Events>();
  auto step = 0;
  for (auto i = 0; i < 20; ++i) {
    if (i >= offsets[step + 1]) {
      ++step;
    }
    evtsWriter(0, ids[step], 0.5f * i, 2.f * i, 3.f * i);
  }
  auto evtTable = builderE.finalize();

  auto bk = std::make_pair(soa::getLabelFromType<aod::Events>(), "fID");
  ArrowTableSlicingCache cache({bk});

  try {
    auto s = cache.updateCacheEntry(0, {evtTable});
  } catch (RuntimeErrorRef re) {
    REQUIRE(std::string{error_from_ref(re).what} == "Table Events index fID is not sorted: next value 3 < previous value 4!");
    return;
  } catch (...) {
    FAIL("Slicing should have failed due to unsorted index");
  }
}
