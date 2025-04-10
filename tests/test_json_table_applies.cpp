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

#include <bm/bm_sim/P4Objects.h>
#include <bm/bm_sim/match_tables.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/phv_source.h>

#include <string>
#include <vector>
#include <memory>

using namespace bm;

class JsonTableAppliesTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  P4Objects objects;

  JsonTableAppliesTest() {
    phv_source = PHVSourceIface::make_phv_source();
  }

  virtual void SetUp() {
    const std::string json_path = TESTDATADIR "/table_applies_example.json";
    std::ifstream json_file(json_path);
    ASSERT_TRUE(json_file);
    const int device_id = 0;
    const auto &cfg_iface = std::unique_ptr<SimpleSwitchMock>(
        new SimpleSwitchMock(device_id));
    LookupStructureFactory factory;
    ASSERT_EQ(0, objects.init_objects(&json_file, &factory));
  }

  // Convenience class to mock SimpleSwitchMock for P4Objects
  class SimpleSwitchMock : public DevMgr, public RuntimeInterface {
   public:
    explicit SimpleSwitchMock(device_id_t device_id)
        : DevMgr(device_id) { }

    // DevMgr
    port_t port_add(const std::string &iface_name, port_t port_num,
                    const PortExtras &port_extras) override {
      (void) iface_name;
      (void) port_num;
      (void) port_extras;
      return 0;
    }

    ReturnCode port_remove(port_t port) override {
      (void) port;
      return ReturnCode::SUCCESS;
    }

    bool port_exists(port_t port) const override {
      (void) port;
      return false;
    }

    ReturnCode port_set_status(port_t port, bool status) override {
      (void) port;
      (void) status;
      return ReturnCode::SUCCESS;
    }

    std::map<port_t, PortInfo> get_port_info() const override {
      return {};
    }

    ReturnCode register_status_cb(const PortStatus &cb) override {
      (void) cb;
      return ReturnCode::SUCCESS;
    }

    // RuntimeInterface
    MatchErrorCode mt_add_entry(
        const std::string &table_name,
        const std::vector<MatchKeyParam> &match_key,
        const std::string &action_name,
        ActionData action_data,
        entry_handle_t *handle,
        int priority = -1) override {
      (void) table_name;
      (void) match_key;
      (void) action_name;
      (void) action_data;
      (void) handle;
      (void) priority;
      return MatchErrorCode::SUCCESS;
    }

    MatchErrorCode mt_set_default_action(
        const std::string &table_name,
        const std::string &action_name,
        ActionData action_data) override {
      (void) table_name;
      (void) action_name;
      (void) action_data;
      return MatchErrorCode::SUCCESS;
    }

    MatchErrorCode mt_delete_entry(
        const std::string &table_name,
        entry_handle_t handle) override {
      (void) table_name;
      (void) handle;
      return MatchErrorCode::SUCCESS;
    }

    MatchErrorCode mt_modify_entry(
        const std::string &table_name,
        entry_handle_t handle,
        const std::string &action_name,
        ActionData action_data) override {
      (void) table_name;
      (void) handle;
      (void) action_name;
      (void) action_data;
      return MatchErrorCode::SUCCESS;
    }

    MatchErrorCode mt_set_entry_ttl(
        const std::string &table_name,
        entry_handle_t handle,
        unsigned int ttl_ms) override {
      (void) table_name;
      (void) handle;
      (void) ttl_ms;
      return MatchErrorCode::SUCCESS;
    }
  };
};

TEST_F(JsonTableAppliesTest, TableAppliesExist) {
  // Check that the table applies were created
  auto table_apply1 = objects.get_control_node_cfg("ipv4_lpm_apply1");
  ASSERT_NE(nullptr, table_apply1);
  auto table_apply2 = objects.get_control_node_cfg("ipv4_lpm_apply2");
  ASSERT_NE(nullptr, table_apply2);
  
  // Check that they are different objects
  ASSERT_NE(table_apply1, table_apply2);
  
  // Check that they point to the same table
  auto table_apply1_obj = dynamic_cast<const TableApply *>(table_apply1);
  auto table_apply2_obj = dynamic_cast<const TableApply *>(table_apply2);
  ASSERT_NE(nullptr, table_apply1_obj);
  ASSERT_NE(nullptr, table_apply2_obj);
  ASSERT_EQ(table_apply1_obj->get_table(), table_apply2_obj->get_table());
}

TEST_F(JsonTableAppliesTest, PipelineInitTable) {
  // Check that the pipeline's init_table is set to the first table apply
  auto pipeline = objects.get_pipeline_cfg("ingress");
  ASSERT_NE(nullptr, pipeline);
  
  // Create a packet to test the pipeline
  auto packet = std::unique_ptr<Packet>(new Packet(
      Packet::make_new(phv_source.get())));
  
  // Apply the pipeline and check that it works
  pipeline->apply(packet.get());
}

TEST_F(JsonTableAppliesTest, ConditionalNextNode) {
  // Check that the conditional's next node is set to the second table apply
  auto conditional = objects.get_conditional_cfg("check_ipv4");
  ASSERT_NE(nullptr, conditional);
  
  // Get the true_next node
  auto true_next = conditional->get_next_node_if_true();
  ASSERT_NE(nullptr, true_next);
  
  // Check that it's the second table apply
  auto table_apply2 = objects.get_control_node_cfg("ipv4_lpm_apply2");
  ASSERT_EQ(true_next, table_apply2);
}
