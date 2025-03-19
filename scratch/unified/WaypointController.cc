#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

class WaypointController {
public:
    WaypointController(Ptr<Node> node, std::vector<Vector> waypoints, double speed)
        : m_node(node), m_waypoints(waypoints), m_speed(speed), m_index(0), m_forward(true) {
        
        m_mobility = m_node->GetObject<WaypointMobilityModel>();
        if (!m_mobility) {
            m_mobility = CreateObject<WaypointMobilityModel>();
            m_node->AggregateObject(m_mobility);
        }
        
        ScheduleNextWaypoint();
    }

private:
    Ptr<Node> m_node;
    Ptr<WaypointMobilityModel> m_mobility;
    std::vector<Vector> m_waypoints;
    double m_speed;
    uint32_t m_index;
    bool m_forward;

    void ScheduleNextWaypoint() {
        if (m_waypoints.empty()) return;

        uint32_t nextIndex;
        
        if (m_forward) {
            nextIndex = m_index + 1;
            if (nextIndex >= m_waypoints.size()) {
                // Reverse direction
                m_forward = false;
                nextIndex = (m_index > 0) ? m_index - 1 : 0;
            }
        } else {
            if (m_index > 0) {
                nextIndex = m_index - 1;
            } else {
                // Reverse direction again
                m_forward = true;
                nextIndex = 1; // Go to second waypoint (if it exists)
                if (nextIndex >= m_waypoints.size()) {
                    nextIndex = 0; // Stay at current if there's only one waypoint
                }
            }
        }

        //std::cout << "Current pos index: " << m_index << ", next pos index: " << nextIndex << std::endl;
        Vector currentPos = m_waypoints[m_index];
        Vector nextPos = m_waypoints[nextIndex];

        double distance = CalculateDistance(currentPos, nextPos);
        double travelTime = std::max(0.001, distance / m_speed); // Ensure minimum travel time
        
        //std::cout << "Moving from " << currentPos << " to " << nextPos << " in " << travelTime << "s" << std::endl;

        // Add waypoint slightly in the future to avoid t_span == 0
        double currentTime = Simulator::Now().GetSeconds();
        m_mobility->AddWaypoint(Waypoint(Seconds(currentTime + travelTime), nextPos));

        m_index = nextIndex;
        Simulator::Schedule(Seconds(travelTime), &WaypointController::ScheduleNextWaypoint, this);
    }

    double CalculateDistance(const Vector &a, const Vector &b) {
        return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2));
    }
};