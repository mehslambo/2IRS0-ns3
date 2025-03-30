import pandas as pd
from pathlib import Path
import xml.etree.ElementTree as ET

def load_node_pos_df(scenario_folder:Path) -> pd.DataFrame:
    node_pos_df = pd.read_csv(scenario_folder / 'CourseChange.csv', delimiter=';', index_col=False)
    node_pos_df["Time"] = node_pos_df["Time"].str.replace('ns', '').astype(float)
    node_pos_df.drop_duplicates(subset=["Time","NodeId"], keep='last', inplace=True)
    node_pos_df.reset_index(drop=True, inplace=True)
    return node_pos_df

def load_phy_df(scenario_file:Path) -> pd.DataFrame:
    phy_df = pd.read_csv(scenario_file, delimiter=';', index_col=False)
    for col in ["Time", "TxBeginFirstSeen", "RxEndLastSeen"]:
        if col in phy_df.columns:
            phy_df[col] = phy_df[col].str.replace('ns', '').astype(float)
    # Drop all columns where SnifferNodeId == ?
    for col in ["SnifferNodeId", "SnifferNodeX", "SnifferNodeY", "SnifferNodeZ", "SnifferNodeType",
                "DestinationNodeId", "DestinationNodeX", "DestinationNodeY", "DestinationNodeZ", "DestinationNodeType",
                "SourceNodeId", "SourceNodeX", "SourceNodeY", "SourceNodeZ", "SourceNodeType"]:
        if col in phy_df.columns:
            phy_df = phy_df[phy_df[col] != "?"]
            phy_df = phy_df[phy_df[col] != "Unknown"]
    for col in ["SnifferNodeId", "SnifferNodeX", "SnifferNodeY", "SnifferNodeZ",
                "DestinationNodeId", "DestinationNodeX", "DestinationNodeY", "DestinationNodeZ",
                "SourceNodeId", "SourceNodeX", "SourceNodeY", "SourceNodeZ"]:
        if col in phy_df.columns:
            phy_df[col] = phy_df[col].astype(float)
    return phy_df

def load_phy_rx_begin(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyRxBegin.csv')
    df = df[df["SnifferNodeId"] == df["DestinationNodeId"]]
    return df

def load_phy_rx_end(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyRxEnd.csv')
    df = df[df["SnifferNodeId"] == df["DestinationNodeId"]]
    return df

def load_phy_rx_drop(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyRxDropWithReason.csv')
    df = df[df["SnifferNodeId"] == df["DestinationNodeId"]]
    return df

def load_phy_tx_begin(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyTxBegin.csv')
    df = df[df["SnifferNodeId"] == df["SourceNodeId"]]
    return df

def load_phy_tx_end(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyTxEnd.csv')
    df = df[df["SnifferNodeId"] == df["SourceNodeId"]]
    return df

def load_phy_tx_drop(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PhyTxDropWithReason.csv')
    df = df[df["SnifferNodeId"] == df["SourceNodeId"]]
    return df

def load_monitor_sniffer_rx(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'MonitorSnifferRx.csv')
    df = df[df["SnifferNodeId"] == df["DestinationNodeId"]]
    return df

def load_monitor_sniffer_tx(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'MonitorSnifferTx.csv')
    df = df[df["SnifferNodeId"] == df["SourceNodeId"]]
    return df

def load_packet_logging_stats(scenario_folder:Path) -> pd.DataFrame:
    df = load_phy_df(scenario_folder / 'PacketLoggingStats.csv')
    
    return df

def load_power_state_df(scenario_folder:Path) -> pd.DataFrame:
    df = pd.read_csv(scenario_folder / 'PowerState.csv', delimiter=';', index_col=False)
    df["Start"] = df["Start"].str.replace('ns', '').astype(float)
    df["Duration"] = df["Duration"].str.replace('ns', '').astype(float)
    df.drop_duplicates(subset=["Start","NodeId"], keep='last', inplace=True)
    df.reset_index(drop=True, inplace=True)
    return df

def load_flow_monitor_xml(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Extract flow statistics from <FlowStats>
    flow_stats = []
    flow_stats_node = root.find('FlowStats')
    if flow_stats_node is not None:
        for flow in flow_stats_node.findall('Flow'):
            # Copy all attributes (they are strings)
            flow_stats.append(flow.attrib)
    
    df_stats = pd.DataFrame(flow_stats)
    
    # Extract classifier information from <Ipv4FlowClassifier>
    classifier_stats = []
    classifier_node = root.find('Ipv4FlowClassifier')
    if classifier_node is not None:
        for flow in classifier_node.findall('Flow'):
            classifier_stats.append(flow.attrib)
    
    df_classifier = pd.DataFrame(classifier_stats)
    
    # Merge both dataframes on 'flowId'
    if not df_classifier.empty:
        df = pd.merge(df_stats, df_classifier, on='flowId', how='left')
    else:
        df = df_stats.copy()
    
    # Convert fields to numeric types
    numeric_fields = ['flowId', 'txBytes', 'rxBytes', 'txPackets', 'rxPackets', 
                      'lostPackets', 'timesForwarded']
    # Process time fields (remove "ns" and any '+' sign)
    time_fields = ['timeFirstTxPacket', 'timeFirstRxPacket', 
                   'timeLastTxPacket', 'timeLastRxPacket',
                   'delaySum', 'jitterSum', 'lastDelay']

    for field in numeric_fields:
        if field in df.columns:
            df[field] = pd.to_numeric(df[field], errors='coerce')

    for field in time_fields:
        if field in df.columns:
            # Remove the "ns" suffix and "+" if present, then convert to float (nanoseconds)
            df[field] = df[field].str.replace("ns", "", regex=False).str.replace("+", "", regex=False)
            df[field] = pd.to_numeric(df[field], errors='coerce')
    
    return df