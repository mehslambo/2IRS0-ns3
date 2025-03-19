import pandas as pd
from pathlib import Path

def load_node_pos_df(scenario_folder:Path) -> pd.DataFrame:
    node_pos_df = pd.read_csv(scenario_folder / 'CourseChange.csv', delimiter=';', index_col=False)
    node_pos_df["Time"] = node_pos_df["Time"].str.replace('ns', '').astype(float)
    node_pos_df.drop_duplicates(subset=["Time","NodeId"], keep='last', inplace=True)
    node_pos_df.reset_index(drop=True, inplace=True)
    return node_pos_df

def load_phy_df(scenario_file:Path) -> pd.DataFrame:
    phy_df = pd.read_csv(scenario_file, delimiter=';', index_col=False)
    phy_df["Time"] = phy_df["Time"].str.replace('ns', '').astype(float)
    # Drop all columns where SnifferNodeId == ?
    for col in ["SnifferNodeId", "SnifferNodeX", "SnifferNodeY", "SnifferNodeZ", "SnifferNodeType",
                "DestinationNodeId", "DestinationNodeX", "DestinationNodeY", "DestinationNodeZ", "DestinationNodeType",
                "SourceNodeId", "SourceNodeX", "SourceNodeY", "SourceNodeZ", "SourceNodeType"]:
        phy_df = phy_df[phy_df[col] != "?"]
    for col in ["SnifferNodeId", "SnifferNodeX", "SnifferNodeY", "SnifferNodeZ",
                "DestinationNodeId", "DestinationNodeX", "DestinationNodeY", "DestinationNodeZ",
                "SourceNodeId", "SourceNodeX", "SourceNodeY", "SourceNodeZ"]:
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