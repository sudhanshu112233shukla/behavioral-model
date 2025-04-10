/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <gtest/gtest.h>

#include <bm/bm_sim/match_tables.h>
#include <bm/bm_sim/match_key_builders.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/phv_source.h>
#include <bm/bm_sim/actions.h>

#include <memory>
#include <string>
#include <vector>

using namespace bm;

namespace {

struct DummyNode : public ControlFlowNode {
  DummyNode(const std::string &name, p4object_id_t id)
      : ControlFlowNode(name, id) { }

  const ControlFlowNode *operator()(Packet *) const override {
    return nullptr;
  }
};

class TableAppliesTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  std::unique_ptr<MatchTable> table{nullptr};
  std::unique_ptr<TableApply> table_apply1{nullptr};
  std::unique_ptr<TableApply> table_apply2{nullptr};
  std::unique_ptr<DummyNode> node1{nullptr};
  std::unique_ptr<DummyNode> node2{nullptr};
  std::unique_ptr<DummyNode> node3{nullptr};
  std::unique_ptr<DummyNode> node4{nullptr};
  ActionFn action_fn;
  ActionFn action_fn_2;
  std::unique_ptr<Packet> pkt{nullptr};
  MatchKeyBuilder key_builder;

  TableAppliesTest()
      : action_fn("action", 0, 0),
        action_fn_2("action_2", 1, 0) {
    phv_source = PHVSourceIface::make_phv_source();
    table = std::unique_ptr<MatchTable>(new MatchTable(
        "test_table", 0, sizeof(int), 1, &key_builder));
    table_apply1 = std::unique_ptr<TableApply>(
        new TableApply("test_table_apply1", 0, table.get()));
    table_apply2 = std::unique_ptr<TableApply>(
        new TableApply("test_table_apply2", 1, table.get()));
    node1 = std::unique_ptr<DummyNode>(new DummyNode("node1", 0));
    node2 = std::unique_ptr<DummyNode>(new DummyNode("node2", 1));
    node3 = std::unique_ptr<DummyNode>(new DummyNode("node3", 2));
    node4 = std::unique_ptr<DummyNode>(new DummyNode("node4", 3));
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
  }

  virtual void SetUp() {
    table->set_next_node(0, node1.get());
    table->set_next_node(1, node2.get());
    table->set_cached_table_apply(table_apply1.get());
    table_apply1->set_next_node(0, node1.get());
    table_apply1->set_next_node(1, node2.get());
    table_apply2->set_next_node(0, node3.get());
    table_apply2->set_next_node(1, node4.get());
  }
};

TEST_F(TableAppliesTest, CachedApply) {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  entry_handle_t handle;
  MatchErrorCode rc = table->add_entry(match_key, &action_fn, ActionData(), &handle, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const ControlFlowNode *next_node = table->apply_action(pkt.get());
  ASSERT_EQ(node1.get(), next_node);
}

TEST_F(TableAppliesTest, NonCachedApply) {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  entry_handle_t handle;
  MatchErrorCode rc = table->add_entry(match_key, &action_fn, ActionData(), &handle, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const ControlFlowNode *next_node = table->apply_action(pkt.get(), table_apply2.get());
  ASSERT_EQ(node3.get(), next_node);
}

TEST_F(TableAppliesTest, MultipleApplies) {
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  entry_handle_t handle;
  MatchErrorCode rc = table->add_entry(match_key, &action_fn, ActionData(), &handle, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  const ControlFlowNode *next_node1 = table->apply_action(pkt.get(), table_apply1.get());
  ASSERT_EQ(node1.get(), next_node1);

  const ControlFlowNode *next_node2 = table->apply_action(pkt.get(), table_apply2.get());
  ASSERT_EQ(node3.get(), next_node2);
}

TEST_F(TableAppliesTest, DifferentActions) {
  std::vector<MatchKeyParam> match_key1;
  match_key1.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x01", 1));
  entry_handle_t handle1;
  MatchErrorCode rc1 = table->add_entry(match_key1, &action_fn, ActionData(), &handle1, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc1);

  std::vector<MatchKeyParam> match_key2;
  match_key2.emplace_back(MatchKeyParam::Type::EXACT, std::string("\x02", 1));
  entry_handle_t handle2;
  MatchErrorCode rc2 = table->add_entry(match_key2, &action_fn_2, ActionData(), &handle2, 1);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc2);

  pkt->get_phv()->get_packet_id() = 1;
  const ControlFlowNode *next_node1 = table->apply_action(pkt.get(), table_apply1.get());
  ASSERT_EQ(node1.get(), next_node1);

  pkt->get_phv()->get_packet_id() = 2;
  const ControlFlowNode *next_node2 = table->apply_action(pkt.get(), table_apply1.get());
  ASSERT_EQ(node2.get(), next_node2);

  pkt->get_phv()->get_packet_id() = 1;
  const ControlFlowNode *next_node3 = table->apply_action(pkt.get(), table_apply2.get());
  ASSERT_EQ(node3.get(), next_node3);

  pkt->get_phv()->get_packet_id() = 2;
  const ControlFlowNode *next_node4 = table->apply_action(pkt.get(), table_apply2.get());
  ASSERT_EQ(node4.get(), next_node4);
}

}  // namespace
