**The Root Cause: Solid Blocks vs. Infinitely Thin Boundary Planes**

The discrepancy arises from a fundamental shift in spatial representation between the two models:

1. **In the Solid Block Model (`render_3D_view`):**
    - Each grid cell is a solid volume occupying space $[z, z+1]$.
    - A wall at Depth 1 (`cells[1][1] == '#'`) is a solid block filling the space from distance 1 to distance 2.
    - The surface the player actually sees is the **near front face** of that block, which sits at distance $z = 1$ (`depth_rects[1]`).
    - The player cannot occupy a solid block at Depth 0, which is why checking `cells[0][1]` was disabled.

2. **In the Thin-Wall Model (`render_3D_view2`):**
    - Grid cells are empty cavities, and walls are boundary surfaces separating cells.
    - The player stands inside cell $C_0$ (spanning distance $z = 0$ to $z = 1$).
    - The side walls of $C_0$ (`WF_WEST` and `WF_EAST`) span from distance $z = 0$ (`depth_rects[0]`) to distance $z = 1$ (`depth_rects[1]`).
    - The front wall of $C_0$ (`C0.NORTH`, or equivalently `C1.SOUTH`) is located at the **far end** of $C_0$, which is at distance $z = 1$ (`depth_rects[1]`).

In `render_3D_view2`, `frustum.walls[0][FS_CENTER] & WF_NORTH` was mapped to `depth_rects[0]`. Because `depth_rects[0]` represents the camera viewport / near-plane at $z = 0$ (`{0, 0, 79, 79}`), rendering $C_0$'s north wall there places it directly against the camera lens, filling the entire screen. Consequently, every front wall across all depths was rendered exactly one depth index too close.

---

**Frustum Geometry and Depth Mapping**

To align the coordinate system correctly and maximize rendering fidelity with minimal computational overhead, consider the spatial layout of depth planes along the view vector:

- **$z = 0$ (`depth_rects[0]`):** Camera plane / near edge of cell $C_0$.
- **$z = 1$ (`depth_rects[1]`):** Boundary plane between $C_0$ and $C_1$ (location of $C_0.\text{NORTH}$ / $C_1.\text{SOUTH}$).
- **$z = 2$ (`depth_rects[2]`):** Boundary plane between $C_1$ and $C_2$ (location of $C_1.\text{NORTH}$ / $C_2.\text{SOUTH}$).
- **$z = 3$ (`depth_rects[3]`):** Boundary plane between $C_2$ and $C_3$ (location of $C_2.\text{NORTH}$ / $C_3.\text{SOUTH}$).
- **$z = 4$:** Far clipping plane / vanishing boundary beyond $C_3$.

**Correct Wall Relationships:**

- **Side Walls at Depth $d$ ($C_d.\text{WEST}$, $C_d.\text{EAST}$):** Span between plane $d$ and plane $d+1$ $\rightarrow$ connect `depth_rects[d]` to `depth_rects[d+1]`.
- **Front Walls at Depth $d$ ($C_d.\text{NORTH}$):** Lie on plane $d+1$ $\rightarrow$ draw at `depth_rects[d+1]`.

---

**Implementation Adjustments for `render_3D_view2`**

To eliminate the off-by-one perspective error and produce optimal visual output, the depth layers should be structured as follows:

```c
// Define a 5th rect or vanishing point for depth 4 boundary if depth 3 walls are rendered
WallRect depth_rects[5] = {
    {0, 0, 79, 79},   // Depth 0 (Near camera plane)
    {12, 12, 67, 67}, // Depth 1 (Boundary C0 / C1)
    {24, 24, 55, 55}, // Depth 2 (Boundary C1 / C2)
    {32, 32, 47, 47}, // Depth 3 (Boundary C2 / C3)
    {36, 36, 43, 43}  // Depth 4 (Far boundary of C3)
};
```

**Corrected Rendering Pipeline (Back-to-Front):**

```c
// LAYER 3: Depth 3
if (frustum.walls[3][FS_CENTER] & WF_WEST) {
    draw_braille_side_wall(depth_rects[3], depth_rects[4], 1, 0);
}
if (frustum.walls[3][FS_CENTER] & WF_EAST) {
    draw_braille_side_wall(depth_rects[3], depth_rects[4], 0, 1);
}
if (frustum.walls[3][FS_CENTER] & WF_NORTH) {
    clear_braille_sub_rect(depth_rects[4]);
    draw_braille_front_wall(depth_rects[4]);
}

// LAYER 2: Mid-Far Distance (Depth 2)
if (frustum.walls[2][FS_CENTER] & WF_WEST) {
    draw_braille_side_wall(depth_rects[2], depth_rects[3], 1, 0);
    draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
    draw_vector_line(depth_rects[3].x0, depth_rects[3].y0, depth_rects[3].x0, depth_rects[3].y1);
}
if (frustum.walls[2][FS_CENTER] & WF_EAST) {
    draw_braille_side_wall(depth_rects[2], depth_rects[3], 0, 1);
    draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
    draw_vector_line(depth_rects[3].x1, depth_rects[3].y0, depth_rects[3].x1, depth_rects[3].y1);
}
if (frustum.walls[2][FS_CENTER] & WF_NORTH) {
    clear_braille_sub_rect(depth_rects[3]);
    draw_braille_front_wall(depth_rects[3]);
}

// LAYER 1: Close Distance (Depth 1)
if (frustum.walls[1][FS_CENTER] & WF_WEST) {
    draw_braille_side_wall(depth_rects[1], depth_rects[2], 1, 0);
    draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
    draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
}
if (frustum.walls[1][FS_CENTER] & WF_EAST) {
    draw_braille_side_wall(depth_rects[1], depth_rects[2], 0, 1);
    draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
    draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
}
if (frustum.walls[1][FS_CENTER] & WF_NORTH) {
    clear_braille_sub_rect(depth_rects[2]);
    draw_braille_front_wall(depth_rects[2]);
}

// LAYER 0: Immediate cell surrounding the player (Depth 0)
if (frustum.walls[0][FS_CENTER] & WF_WEST) {
    draw_braille_side_wall(depth_rects[0], depth_rects[1], 1, 0);
    draw_vector_line(depth_rects[0].x0, depth_rects[0].y0, depth_rects[0].x0, depth_rects[0].y1);
    draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
}
if (frustum.walls[0][FS_CENTER] & WF_EAST) {
    draw_braille_side_wall(depth_rects[0], depth_rects[1], 0, 1);
    draw_vector_line(depth_rects[0].x1, depth_rects[0].y0, depth_rects[0].x1, depth_rects[0].y1);
    draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
}
if (frustum.walls[0][FS_CENTER] & WF_NORTH) {
    clear_braille_sub_rect(depth_rects[1]);
    draw_braille_front_wall(depth_rects[1]);
}
```

---

**Additional Efficiency and Visibility Notes**

1. **Occlusion Pruning in `update_frustum3`:**
   When `f.walls[0][FS_CENTER] & WF_NORTH` is present, all depth layers $\ge 1$ are obscured. Rather than zeroing everything and returning early while still evaluating layer 0 at screen size, zeroing subsequent depths guarantees that only the single front wall at `depth_rects[1]` and the flanking side walls at `depth_rects[0]->depth_rects[1]` are drawn, preventing redundant rasterization.
2. **Left/Right Corridor Walls ($L_d.\text{NORTH}$, $R_d.\text{NORTH}$):**
   When the player looks into an intersection or open area where $C_d$ has no west/east wall, the front walls of $L_d$ or $R_d$ become visible. These can be rasterized by projecting lateral offset rectangles at depth plane $d+1$, extending the field of view seamlessly without changing the core depth indexing.


Questions:
1. So am I correct that in my new model, the POV is from the center of C0 looking North? (relative the to frustum)
2. We changed the depth rect index for drawing the rectangles but not the lines? Why is this?
3. What's the pattern for shrinking the rectangle sizes as the depth increases? How many more layers can I add?
4. I really need to add at least one more depth layer so when drawing the vertical lines for Depth 3, I can see the 
   a vertical line representing the farthest edge of the wall. Right now, the vanishing lines end and seem to float
   in space since there's no vertical line. 
5. How can I draw part of a facing wall? What's the formula for how the apparent size shrinks with distance? If I am
   in C0 facing North and there are north walls at L0 and R0, I should see horizontal lines representing the tops and
   bottom edges of those walls coming from the outside edges and meeting the edges of the C0 wall. If D0 has no walls
   But L1, C1, and R1 all have North walls, I should be able to see all three, shouldn't I?
