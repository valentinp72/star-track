# Mechanics

All dimensions in mm. Heights are measured from the ground.

## Current build

Dimensions taken from the STEP export of the Fusion 360 design
("Aluminium telescope mount"), corrected with measurements of the real parts. The
telescope tube is not in that export; its size comes from the earlier Fusion 360
design. Heights in this table are from the bottom of the extrusions; the feet add
about 50.

| Part | Description |
| --- | --- |
| T base | Two 30×60 aluminium extrusions (Motedis type B, slot 8) standing on edge (60 tall): a 650 crossbar and a 350 stem butting against the middle of it. Two 60×60 angle brackets at the junction. |
| Feet | Three ball-joint levelling feet ("Pied 40", M8 × 80 threaded rod, Motedis series 15 PA), one at each end of the T, in the bottom slot. Screwed in fully they raise the extrusions about 50; adjustable. |
| Azimuth support | 95 × 70 × 9.5 plate on top of the stem, at the middle of the stem (175 from the crossbar). Four 30×30 blocks (type B, slot 8), about 30 long, two on each side of the stem under the plate, held with 90° brackets and hammer T-nuts, so they can be moved. |
| Azimuth bearing | "95x70 turntable" case: two 95 × 70 × 9.5 plates with an axial thrust ball bearing between them (heights 60–79). |
| Azimuth drive | NEMA 17 on an 82 × 39.5 × 3 plate, fixed to the base about 132 from the axis. GT2 pulley with 250 teeth (Ø158.6, Ø161 over the flanges, 10 thick) on the turning part, at height 79–89. |
| L | 45×90 extrusion, cut from one 1000 bar: a 380 bar lying flat (90 horizontal, height 89–134), centred on the azimuth axis, and a 620 arm standing on one end of it (height 134–754), its wide faces along the bar. Motedis 45×90 type B, slot 10, two core holes that take M12. One 85 × 85 corner bracket, 90 wide, in the inner corner. |
| Altitude bearing | Second "95x70 turntable" case on the inner wide face of the arm, centred 120 below the top of the arm (height ≈ 634). |
| Altitude drive | NEMA 17 on a motor plate beside the arm, about 150 below the altitude axis. GT2 pulley with 250 teeth and holes to attach the tube. |
| Telescope | Tube 900 long, about Ø145, hanging over the 380 bar, fixed to the altitude pulley with "tube attach" blocks. |
| Electronics case | 90 × 150 × 73 case on the outer face of the arm near its top (height ≈ 604–776): single-board computer, 5-inch HDMI screen, emergency stop button. |

## Problem: play in the bearings

- An axial thrust bearing only carries load along its axis. In this L layout:
  - the altitude bearing carries the tube's weight sideways (radial load) plus a
    twisting moment, because the tube hangs to one side of the arm;
  - the azimuth bearing carries the weight plus a tipping moment whenever the centre
    of gravity of the turning part is not on the axis.
- The angle the tube can rock through is about gap ÷ length. Example: a 0.05 gap over
  a 15 long seat gives about 11′. At 150× (6 mm eyepiece on a 900 focal length) the
  field of view is about 20′ and Jupiter is about 45″ across.
- To keep movement under a few arc-seconds the gap would have to be 1–2 µm. Only
  preload (bearings clamped against each other) reaches that; spacing the bearings
  apart reduces whatever remains.

## Redesign: the same bearing kit on both axes

Each axis gets a 20 mm steel shaft through the 45 mm thickness of the bar, with a
flange bearing bolted on each wide face. The thrust bearings and the 95x70 cases are
no longer used.

### Flange bearing UCFL204GP

Gopart brand, sold by Kramp. Dimensions as listed by retailers (to be checked on the
real part): bore 20, length 113, width 60, height 33.3, flange 11 thick, two Ø12 bolt
holes 90 apart, cast iron housing, self-aligning insert, grease nipple, two set
screws.

### Mounting on a 90 wide face

- A 90 wide face has two slots 45 apart and no slot in the middle.
- The bearing is turned 30° from the length of the bar: each bolt then lands in one
  slot, about 39 either side of the hole along the bar.
- M8 bolts with large washers (DIN 9021) and T-nuts: 45-series slots usually take
  M8, while the bearing holes are Ø12.
- Ø24 hole through the 45 thickness, in the middle of the face between the slots.
  The bearings locate the shaft, so the hole does not need to be precise.
- With the bearing centre about 15 from its mounting face, the two bearings of an
  axis are about 75 apart.

### Fitting (same on both axes)

1. Drill the Ø24 hole (step drill, from both sides).
2. Bolt a bearing on each face, bolts snug. Push the shaft through, then tighten the
   bolts; the self-aligning inserts line up with the shaft.
3. Tighten the set screws of one bearing. Push the other bearing's inner ring firmly
   toward the bar and tighten its set screws. This removes the play.
4. Azimuth only: clamp the shaft collar just below the lower bearing before fitting
   the L; it carries the weight.

Thread-locker on the set screws keeps them from loosening.

### Azimuth

- The shaft stands in a round-flange shaft holder STHRB20 (aluminium, 32 long,
  Ø28 hub, Ø50 × 8 flange with 4 × Ø5.5 holes on Ø39, two M5 set screws) on the
  existing 95 × 70 plate.
- A printed ring (8 thick, Ø51 inside, Ø88 outside) lies around the holder's
  flange. Four M5 × 20 socket head screws (DIN 912) with printed knobs go through
  the pulley web (on Ø68, at 45°) and the ring into M5 holes tapped in the plate:
  the pulley clamps the holder's flange onto the plate, so the mount head lifts off
  without tools. The screws sit outside the holder because a Ø24 knob does not fit
  next to its Ø28 hub. Each screw goes about 6 into the 9.5 plate.
- Knob ([`cad/knob-m5.solidcraft`](../cad/knob-m5.solidcraft), STL next to it):
  Ø24 × 10, six finger scallops, a Ø8.4 × 4.8 pocket for the screw head and a
  3.8 hex key that goes into the head's 4 socket. Press the head in with a vice
  (or warm it first); the head stands 0.2 proud, so the screw, not the knob,
  clamps the pulley. Printed top down, no supports.
- A 20 × 32 × 14 shaft collar (Kramp "stelring", listed by a reseller as 705A20)
  under the lower bearing carries the weight.
- The printed 250-tooth pulley (the old one is broken) lies on the holder's
  flange, its Ø28.5 bore over the hub. The motor torque goes through the four
  screws into the plate, not through the holder's set screws. The pulley lifts off
  with the head (it rests on the holder's flange), so the belt keeps its tension;
  the ring stays on the plate.
- The azimuth motor moves from the base to under the far end of the 380 bar and
  turns with the L. It sits 152.4 from the axis: with a 20-tooth motor pulley, that
  is the centre distance of a 610 closed GT2 belt. The battery and the electronics
  are on the L too, so no cable crosses the turning joint.
- The motor's face is bolted to a Motedis NEMA 17 motor plate (82 × 39.5 × 3
  aluminium), which hangs from the bar's bottom slots on a printed spacer (80 tall,
  four M6 × 20 screws and T-nuts). Sliding the spacer along the bar tensions the belt.
- Turning the head, the lowest part (the motor pulley) clears the T by about 10.
- The bearing stack is taller than the current case: in the model, standing on its
  feet, the 380 bar sits at height 223–268.

### Altitude

- The shaft crosses the arm 70 below its top (height ≈ 818 in the model).
- Pulley and motor move to the outer face of the arm. The pulley (the same part as
  on azimuth) is bolted with four M5 × 20 screws and nuts, through its Ø39 holes, to
  a second STHRB20 holder on the outer end of the shaft, hub toward the bearing
  (about 3 clear of the outer bearing's insert).
- The motor sits 152.4 below the shaft (610 closed GT2 belt, as on azimuth), on a
  Motedis motor plate held 63 from the arm by a printed spacer bolted into the
  arm's slots. Sliding the spacer down tensions the belt.
- On the inner face, a third STHRB20 holder carries a 100 × 121 × 8 aluminium
  plate with the original Omegon tube clasp. The plate has 4 M5 tapped holes on
  Ø39 for the holder (at 45°, M5 × 20 screws cut to 16 so they stay flush with the
  clasp side) and the clasp's two Ø6.6 holes 60 apart. The tube can
  slide −60 / +40 in the clasp for balancing, and comes out when the clasp is
  opened.
- The motor torque reaches the tube through the two altitude holders' set screws.
  Set screws barely bite a hardened shaft, so the shafts are plain bright steel
  (C45 or S235, Ø20 h9): drill a shallow seat for each set screw, add
  thread-locker, and fix the two altitude holders with retaining compound (for
  example Loctite 638 or 648). The altitude shaft never has to come apart.
- A new, smaller electronics box without a screen sits lower on the outer face
  (90 × 173 × 62, see [Electronics box](#electronics-box)), because the outer
  bearing and pulley take the current case's place.
- The joystick and buttons are in a hand-held unit on a cable, not on the mount.
- Near the zenith the back of the tube clears the top of the azimuth shaft by
  about 97 (worst balancing position).
- Shafts: two Ø20 × 200 rods, not cut. Azimuth: from the base plate to about 18
  above the upper bearing. Altitude: flush with the tube-side holder, sticking out
  about 16 on the pulley side, through the pulley's bore.

### Battery

- LiFePO4 12.8 V 6 Ah. Sizes vary. The model uses 151 × 65 × 95, the usual size of
  a 12 V 7 Ah lead-acid case (about 0.8 kg for LiFePO4).
- At the far end of the 380 bar, opposite the arm. It sits in a printed cradle that
  hooks over the end of the bar, bolts into the bar's two top slots with T-nuts,
  and holds the battery with straps.
- It overhangs the end of the bar by about 4 cm, so it stays about 5 cm clear of the
  tube rings when the tube points at the zenith.
- It sits on the other side of the azimuth axis from the arm, so it partly balances
  the arm's weight.
- Its cable runs along the side of the bar and up the arm to the case. Wiring and
  runtime: [architecture](architecture.md#power).

### Electronics box

[`cad/electronics-box.solidcraft`](../cad/electronics-box.solidcraft), STLs next
to it (`electronics-box.stl`, `electronics-box-lid.stl`).

- 90 wide, like the arm, 173 tall, 59 deep plus a 3 lid. Walls 3, back 4.1. It
  stands on the outer 90 face of the arm, its long side vertical, the lid facing
  away from the arm.
- Four Ø5.5 holes in the back, on the arm's two slots (45 apart), 84 apart
  vertically: M5 screws into slot-10 T-nuts, put in from inside with the lid off.
- Main PCB on four M3 heat-set inserts in standoffs 6 tall (back of the PCB 10 from the
  back face, room for 27 of parts on top). Its USB edge faces a side wall, the
  screw terminals face down.
- That side wall has two Ø3.2 holes for the 3 mm LEDs, which lie on the PCB and
  stick out 6.7 past its edge (hole centres 1.5 above the PCB surface), and an opening for the ESP32 board's USB
  port nearest the GPS and hand-unit pins (the end of the ESP32 board is about
  0.85 past the PCB edge). There the wall is thickened inwards to 0.15 from the
  port: a 13.5 × 9 pocket from outside (R2 corners, 1.5 chamfer on its rim)
  takes the plug's moulding, and a 10 × 4.8 hole through the remaining 0.6
  skin takes its metal shell. Port centre as
  measured: about 5.5 above the PCB surface, and 17.6 from the PCB edge on the
  GPS side (its side 13.1 from that edge).
- Top: Ø22.5 hole for the E-stop, 31 from the back face and 11 off the centre
  line, away from the GPS. The box is 62 deep with the lid so that the E-stop's
  Ø60 legend plate fits on the flat of the top face: 1 from the front and back
  edges, and its side edge where the corner rounding starts. The 38 × 30
  contact block, about 45 tall, goes in with its 30 side across the box (38
  front to back, inside the 4.1–59 depth); it then ends 1.2 from the GPS
  antenna fins.
- GPS board on four M3 inserts (30 × 20) on the back wall, beside the E-stop.
- GPS antenna (25.46 × 25.16 board) pressed between two 1.6 thick fins under the
  top wall, 25.4 apart, patch towards the top wall, its back edge against the
  back wall. The fins are 8 deep.
- Bottom: a hole for the female XT60 (16.1 × 8.5 outside; hole 16.5 × 8.9). As
  on the Amass XT60-F drawing, both corners of one short end are chamfered,
  2.9 along by 2.6 across, leaving a 3.7 flat. A 1.6 thick lip, 5 tall, goes
  round the hole inside, so the socket sits in 8 of plastic, held with hot glue.
- Bottom: a Ø15 hole for the GX16-8 socket, with two Ø3.4 holes 21 apart for its
  M3 screws.
- Motor cables leave through a 9 × 16 slot in the back wall, in line with one arm
  slot, and run hidden in that slot.
- Lid: four M3 screws (socket, pan or button head) into inserts in the corner
  columns, heads in Ø6.5 × 3 counterbores. The lid is 5 thick at the screws.
- Printed on its back, no supports.

### Printed parts

- Printed: pulleys, motor spacers, azimuth pulley ring, battery cradle,
  electronics case, drilling templates, cable guides.
- The motors are not carried by printed parts in bending: each one is bolted to an
  aluminium plate, and the printed spacer between the plate and the L is clamped by
  bolts, so it mostly works in compression. Belt pull is about 10–60 N; a printed
  bracket flexing 0.05 at the motor would turn the axis by about 2′ each time the
  torque reverses.
- Files: [`cad/pulley-gt2-250t.solidcraft`](../cad/pulley-gt2-250t.solidcraft)
  and its STL (three bodies that overlap by 0.1, fused by the slicer; about 160 g
  of PETG, no supports), [`cad/azimuth-motor-spacer.stl`](../cad/azimuth-motor-spacer.stl),
  [`cad/altitude-motor-spacer.stl`](../cad/altitude-motor-spacer.stl),
  [`cad/azimuth-pulley-ring.stl`](../cad/azimuth-pulley-ring.stl),
  [`cad/knob-m5.stl`](../cad/knob-m5.stl). The spacers take M5 heat-set inserts
  for the plate. Their four Ø6.6 holes to the extrusion have Ø13 × 2.5
  counterbores for an M6 head and a DIN 125 washer, which leaves 7.5 of
  plastic, so an M6 × 20 screw reaches about 6 into a slot-10 T-nut.
- The pulley's centre (Ø28.5 bore, 4 × Ø5.5 on Ø39) follows the STHRB20 table
  from the seller; check it against the real holder before printing.
- Not printed: anything that carries the telescope's weight in bending (bearing
  housings, shaft supports, the link between tube and shaft). Plastic creeps under
  constant load and the play would come back.
- Material: PETG or ASA. PLA softens in the sun and creeps.
- Every printed part must fit the 180 × 180 × 180 bed of a Bambu Lab A1 mini. The
  largest is the 250-tooth pulley: Ø159.2 pitch, Ø168 over the flanges, which
  leaves about 6 on each side (no room for a wide brim). The battery cradle
  (159 long) and the electronics box (173 tall, printed on its back) also fit.

## Base stability

- Feet: three levelling feet, 20 from each end of the T (in the model: 305 either
  side of the stem on the 650 crossbar, and 345 from the crossbar along the stem).
- Azimuth axis 95 from the crossbar's inner face (as in the model): about 11 cm of
  margin toward the crossbar and about 15.5 cm toward each of the two long sides of
  the support triangle. The axis is close to the triangle's centre (5 from it).
- The plate and its blocks are held by T-nuts in the stem's slot, so the axis can be
  moved by sliding them along the stem.

## Transport

Three pieces, separated without tools:

| Piece | Joint | Size in the model |
| --- | --- | --- |
| Base (T with feet, plate and ring) | four knob screws through the azimuth pulley | ≈ 65 × 38 × 12 cm |
| Mount head (L with bearings, shafts, motors, belts, electronics case, battery, clasp) | — | ≈ 77 × 49 × 17 cm |
| Telescope (tube) | the knob of the tube clasp | ≈ 90 × 15 × 17 cm |

- The bearings stay assembled, so the preload is kept. The azimuth shaft, its
  holder and the 250-tooth pulley come off with the head; the belts keep their
  tension.
- The knob screws go into four M5 holes tapped in the 95 × 70 plate.
- Re-assembly does not need to be precise: the star alignment at the start of each
  session absorbs small shifts.
- The battery travels on the head, so nothing has to be plugged in between the
  pieces except the hand unit.
- If the head is too tall for the car, the arm can be made removable by replacing the
  corner bracket's bolts with knob bolts.

## Parts list

Prices are rough estimates and were not checked against current listings.

| Part | Altitude | Azimuth | ≈ Price | Source |
| --- | --- | --- | --- | --- |
| Flange bearing UCFL204GP | 2 | 2 | not checked | Kramp |
| Bright steel rod Ø20 h9 (C45 or S235), 200 long, not hardened | 1 | 1 | — | — |
| Round-flange shaft holder STHRB20 (20 mm bore, set screws) | 2 | 1 | — | AliExpress |
| Shaft collar 20 × 32 × 14 | – | 1 | €2.50 | Kramp |
| M8 bolts, DIN 9021 washers, 45-series T-nuts (bearings) | 4 | 4 | €5 per bag | — |
| Closed GT2 belt 610, 6 wide | 1 | 1 | — | existing |
| GT2 pulley 20 teeth, 5 bore | 1 | 1 | — | existing |
| NEMA 17 17HS4401S | 1 | 1 | — | existing |
| GT2 pulley 250 teeth, motor spacer (PETG) | 1 | 1 | filament | printed |
| Pulley ring (PETG) | – | 1 | filament | printed |
| NEMA 17 motor plate, 3 mm aluminium | 1 | 1 | — | existing (Motedis) |
| M5 heat-set inserts and M5 × 10 screws (plate to spacer), M3 × 8 screws (motor to plate) | 3 + 4 | 3 + 4 | €5 | — |
| M6 × 20 screws (DIN 912), DIN 125 washers, slot-10 M6 T-nuts (spacer to L) | 4 | 4 | €4 | — |
| M5 × 20 screws and nuts, pulley to holder | 4 | – | €1 | — |
| M5 × 20 screws cut to 16, holder to clasp plate | 4 | – | — | — |
| Retaining compound (Loctite 638 or 648), thread-locker | 1 | – | €10 | — |
| Aluminium plate 100 × 121 × 8 for the tube clasp | 1 | – | not checked | — |
| M5 × 20 screws (DIN 912) with printed knobs (PETG) | – | 4 | filament | printed |

The battery is listed in [architecture](architecture.md#power), the electronics in
its [parts list](architecture.md#parts-list).

## 3D model

[`3d/index.html`](3d/index.html) shows the redesigned mount: camera presets, a
see-through mode for the drilled holes, an exploded view, sliders for azimuth and
altitude, and a "pack for transport" layout with the size of each piece. Clicking a
part shows what it is and whether it is bought, printed or existing.

Geometry is simplified (profiles, bearings, shaft holders, pulleys, motors, belts,
battery, case and tube as basic shapes). Fasteners other than the bearing bolts and
knobs are not drawn, and neither are the motor cables.
