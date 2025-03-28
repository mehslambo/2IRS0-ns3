#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace ns3;

class WaypointController {
public:
    WaypointController(Ptr<Node> node, std::vector<Vector> waypoints, double speed, double stopTime)
        : m_node(node), m_waypoints(waypoints), m_speed(speed), m_index(0), m_forward(true) {
        
        m_mobility = m_node->GetObject<WaypointMobilityModel>();
        if (!m_mobility) {
            m_mobility = CreateObject<WaypointMobilityModel>();
            m_node->AggregateObject(m_mobility);
        }
        
        ScheduleAllWaypoints(stopTime);
    }

private:
    Ptr<Node> m_node;
    Ptr<WaypointMobilityModel> m_mobility;
    std::vector<Vector> m_waypoints;
    double m_speed;
    uint32_t m_index;
    bool m_forward;

    void ScheduleAllWaypoints(double stopTime) {
        if (m_waypoints.empty()) return;
        
        double currentTime = Simulator::Now().GetSeconds();
        Vector currentPos = m_waypoints[m_index];
        
        // Add the initial position at time zero.
        if (m_index == 0 && currentTime == 0.0) {
            m_mobility->AddWaypoint(Waypoint(Seconds(currentTime), currentPos));
        }
        
        // Loop until the next waypoint would exceed stopTime.
        while (currentTime < stopTime) {
            uint32_t nextIndex;
            if (m_forward) {
                nextIndex = m_index + 1;
                if (nextIndex >= m_waypoints.size()) {
                    // Reverse direction if at the end.
                    m_forward = false;
                    nextIndex = (m_index > 0) ? m_index - 1 : 0;
                }
            } else {
                if (m_index > 0) {
                    nextIndex = m_index - 1;
                } else {
                    // Reverse direction if at the beginning.
                    m_forward = true;
                    nextIndex = (m_waypoints.size() > 1) ? 1 : 0;
                }
            }
            
            Vector nextPos = m_waypoints[nextIndex];
            double distance = CalculateDistance(currentPos, nextPos);
            double travelTime = std::max(0.001, distance / m_speed); // Ensure a minimum travel time
            
            // Do not schedule if the next waypoint would be after stopTime.
            if (currentTime + travelTime > stopTime) {
                break;
            }
            
            currentTime += travelTime;
            std::cout << "[WaypointController] Scheduled waypoint " << nextIndex << " at time " << currentTime << "s." << std::endl;
            m_mobility->AddWaypoint(Waypoint(Seconds(currentTime), nextPos));
            
            m_index = nextIndex;
            currentPos = nextPos;
        }
    }

    double CalculateDistance(const Vector &a, const Vector &b) {
        return std::sqrt(std::pow(b.x - a.x, 2) +
                         std::pow(b.y - a.y, 2) +
                         std::pow(b.z - a.z, 2));
    }
};
