# SimNanite[![License](https://img.shields.io/github/license/ShawnTSH1229/SimNanite.svg)](https://github.com/ShawnTSH1229/XEngine/blob/master/LICENSE)
Simplified Nanite Implementation in MiniEngine

<p align="center">
    <img src="/Resource/logo.png" width="60%" height="60%">
</p>

[<u>**SimNanite Development Blog**</u>](https://shawntsh1229.github.io/2024/05/08/Simplified-Nanite-Virtualized-Geometry-In-MiniEngine/)

# Introduction

This is a simplified Nanite implementation **(SimNanite)** based on Unreal's Nanite virtual geometry. We implement **most of Unreal Nanite's features**.

In offline, we partition the triangles into **clusters** with the **Metis** graph partition library. Then, SimNanite partition the clusters into **cluster groups**, builds the **DAG** (Directed Acyclic Graph) and **BVH tree** based on the cluster groups. In order to avoid the **LOD crack**, SimNanite simplify the mesh globally rather than simplify the triangles per cluster.

At runtime, we implement a three-level GPU culling pipeline, **instance culling**, **BVH node culling** and **cluster culling**. In the BVH node culling pass, we use the **MPMC ( Multiple Producers, Single Consumer )** model to transverse the BVH structure and **integrate the cluster culling** into the BVH culling shader for **work balance**. 

We generate an **indirect draw** or **indirect dispatch** command for those clusters that pass the culling. If the cluster is small enough, we employ the **software rasterization** with compute shader.

During the rasterization pass, we write the **cluster index and triangle index** to the **visibility buffer**. In the next base pass or GBuffer rendering, we fetch these indices from the visibility buffer, calculate the **pixel attributes** (UV and normal) by **barycentric coordinates** and render the scene with these attributes.

# Getting Started

1.Cloning the repository with `git clone https://github.com/ShawnTSH1229/SimNanite.git`.

2.Compile and run the visual studio solution located in `SimNanite\MiniEngine\SimNanite\SimNanite.sln`

# Source Tree

## SimNaniteBuilder

Offline nanite mesh builder, located at MiniEngine\SimNanite\SimNaniteBuilder

SimNanitePartioner.cpp : Partition the triangle into clusters and partition the clusters into cluster groups

SimNaniteMeshSimplifier.cpp : Simplify the mesh based on the meshoptimizer library

SimNaniteBuildBVH.cpp : Build the cluster group BVH tree

SimNaniteBuildDAG.cpp : Build the cluster group Directed Acyclic Graph


## SimNaniteRuntime

located at MiniEngine\SimNanite\SimNaniteRuntime

SimNaniteInstanceCulling.cpp : instance-level culling

SimNanitePersistentCulling.cpp : MPMC BVH node culling and cluster culling

SimNaniteRasterization.cpp : Hardware rasterization and software rasterization

SimNaniteBasePassRendering.cpp : Rendering the scene based on the visibility buffer

# Visualization

Key 0: Visualize Clusters

Key 1: Update Freezing Data

Key 2 / 5: decrease/increase cluster visualize lod

Key 6: Visualize Node Culling Result( set DEBUG_CLUSTER_GROUP to 1)

Kay 7: Visualize Instance Culling Result

# Screen Shot

Clusters Partition

Clusters LOD0:
<p align="center">
    <img src="/Resource/vis_clu_0.png" width="60%" height="60%">
</p>

Clusters LOD1:
<p align="center">
    <img src="/Resource/vis_clu_1.png" width="60%" height="60%">
</p>

Clusters LOD2:
<p align="center">
    <img src="/Resource/vis_clu_2.png" width="60%" height="60%">
</p>

Instance Culling:

<p align="center">
    <img src="/Resource/ins_cull_scene_view.png" width="60%" height="60%">
</p>

Instance Culling:

<p align="center">
    <img src="/Resource/ins_cull_scene_view.png" width="60%" height="60%">
</p>

Visibility Buffer:

<p align="center">
    <img src="/Resource/visibility_buffer.png" width="60%" height="60%">
</p>

Hardware Rasterization:

<p align="center">
    <img src="/Resource/vis_softraster_hard.png" width="60%" height="60%">
</p>


Software Rasterization:

<p align="center">
    <img src="/Resource/vis_softraster_soft.png" width="60%" height="60%">
</p>

Hardware + Software Rasterization Visibility BUffer:

<p align="center">
    <img src="/Resource/vis_softraster_visbuffer.png" width="60%" height="60%">
</p>

BasePass Rendering + Material Type 0:

<p align="center">
    <img src="/Resource/final0.png" width="60%" height="60%">
</p>


BasePass Rendering + Material Type 1 + 0:

<p align="center">
    <img src="/Resource/final1.png" width="60%" height="60%">
</p>

