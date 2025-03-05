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
    phy_df = phy_df[phy_df["DestinationNodeId"] != "?"]
    phy_df = phy_df[phy_df["SourceNodeId"] != "?"]
    phy_df["DestinationNodeId"] = phy_df["DestinationNodeId"].astype(int)
    phy_df["SourceNodeId"] = phy_df["SourceNodeId"].astype(int)
    phy_df["TransmitterNodeId"] = phy_df["TransmitterNodeId"].astype(int)
    phy_df["ReceiverNodeId"] = phy_df["ReceiverNodeId"].astype(int)
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
    df = load_phy_df(scenario_folder / 'PhyRxDrop.csv')
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
    df = load_phy_df(scenario_folder / 'PhyTxDrop.csv')
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