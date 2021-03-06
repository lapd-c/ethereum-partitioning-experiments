#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <fstream>

typedef boost::property<boost::edge_weight_t, uint32_t> EdgeWeightProperty;
// typedef boost::property<boost::vertex_index_t, uint32_t> VertexProperty;

struct VertexProperty {
  explicit VertexProperty(const uint32_t vertex_id = 0,
                          const uint32_t vertex_weight = 0) {
    m_vertex_id = vertex_id;
    m_vertex_weight = vertex_weight;
  };
  uint32_t m_vertex_id;
  uint32_t m_vertex_weight;
};
bool operator==(const VertexProperty &lhs, const VertexProperty &rhs);
bool operator!=(const VertexProperty &lhs, const VertexProperty &rhs);
std::ostream &operator<<(std::ostream &os, const VertexProperty &p);

// out edges, graph vertices, direction, vertexProp, edgeProp, edgeStorage
typedef boost::adjacency_list<boost::hash_setS, boost::hash_setS,
                              boost::undirectedS, VertexProperty,
                              EdgeWeightProperty, boost::hash_setS>
    Graph;

typedef boost::graph_traits<Graph> GraphTraits;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_it;
typedef Graph::vertex_descriptor Vertex;
typedef Graph::edge_descriptor Edge;
typedef GraphTraits::out_edge_iterator out_edge_it;

#define TIME_GAP_LOG 60 * 60 * 4 // 4 hours

class Config {
public:
  // std::map<uint32_t, Vertex> m_id_to_vertex;

  enum {
    HASH_PARTITIONER,
    METIS_PARTITIONER,
    HMETIS_PARTITIONER,
    FACEBOOK_PARTITIONER,
    FILE_PARTITIONER,
    FUTURE_PARTITIONER,
    PART_GRAPH_PARTITIONER
  };

  enum {
    PERIODIC_PARTITIONING,
    DYNAMIC_PARTITIONING
  };

  enum {
    CREATE_TYPE,
    CALL_TYPE,
    CALLCODE_TYPE,
    DELEGATECALL_TYPE,
    STATICCALL_TYPE,
    PRECOMPILED_TYPE,
    OPSELFDESTRUCT_TYPE
  };

  std::string FILE_PATH;
  std::fstream FILE_INPUT;
  std::string GRAPH_SIZE_EVOLUTION_PATH;
  std::string GRAPH_CC_PATH;

  uint32_t N_PARTITIONS = 2;
  uint8_t PARTITIONING_MODE = HASH_PARTITIONER;
  bool SAVE_PARTITIONING = false;
  bool AVOID_PRECOMPILED = false;
  bool DELETE_VERTICES = false;

  uint32_t TIME_REPARTITION = 60 * 60 * 24 * 15;       // 15 days
  uint32_t TIME_REPARTITION_WINDOW = 60 * 60 * 24 * 2; // 2 days
  float CROSS_PARTITION_THRESHOLD = 0.3; // Threshold for when trigger repartition
  float BALANCE_THRESHOLD = 1.3;
  uint8_t PARTITIONING_TYPE = PERIODIC_PARTITIONING;

  Config(const std::string &config_file);

  Config() = delete;
  Config(Config const &) = delete;
  Config(Config &&) = default;
};
std::ostream &operator<<(std::ostream &os, const Config &c);