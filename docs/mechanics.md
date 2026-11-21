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

Thread-locker on the set screws keeps them from loosening.

### Azimuth

- The shaft stands in a round-flange shaft holder STHRB20 (aluminium, 32 long,
  Ø28 hub, Ø50 × 8 flange with 4 × Ø5.5 holes on Ø39, two M5 set screws) on the
  existing plate, now cut to 90 × 90 × 6.7 (threaded for the screws below). The
  plate sits on top of the 30×60 stem, centred on it, held by two 30×30 angle
  brackets with T-nuts in the stem's slots; the 30×30 blocks are no longer used.
- No shaft collar: the set screws of the two bearings' inner rings hold the head
  on the shaft.
- A printed ring (8 thick, Ø51 inside, Ø88 outside) lies around the holder's
  flange. The printed 250-tooth pulley (the old one is broken) lies on the flange
  and the ring, its Ø28.5 bore over the hub.
- Four M6 screws on a 48 × 48 square (Ø68, at 45°), with DIN 125 washers, go
  through the pulley web and the ring (Ø6.6 holes) into the plate's threads: they
  fix the pulley to the plate and take the motor torque. Washer, web and ring add
  15.6, so an M6 × 22 fills the 6.7 plate; an M6 × 25 sticks out 2.7 below it.
- The holder is held to the base by four M5 socket head screws (DIN 912) through
  its flange: two into the plate's threads (M5 × 20), two into hammer-head
  T-nuts. Their heads (Ø8.4 × 5) sit in four Ø9.5 notches in the pulley, on Ø39
  and open to its bore, on the square's centre lines between the M6 screws. The
  pulley does not touch them, and a hex key or the whole screw passes through
  the notch. The pulley still centres on the hub on the four arcs of the bore
  between the notches.
- The azimuth motor moves from the base to under the arm end of the 380 bar, below
  the arm and the electronics box, and turns with the L. Its cable to the box
  stays short; the battery on the other end partly balances it. It sits 152.4 from the axis: with a 20-tooth motor pulley, that
  is the centre distance of a 610 closed GT2 belt. The battery and the electronics
  are on the L too, so no cable crosses the turning joint.
- The motor's face is bolted to a Motedis NEMA 17 motor plate (82 × 39.5 × 3
  aluminium), which hangs from the bar's bottom slots on a printed spacer (80 tall,
  four M6 × 20 screws and T-nuts). Sliding the spacer along the bar tensions the belt.
- Turning the head, the lowest parts (the motor and its pulley) clear the T by
  about 7.7 in the model; setting the bearings higher on the shaft adds to it.
- The bearing stack is taller than the current case: in the model, standing on its
  feet, the 380 bar sits at height 220–265. The bearings slide on the shaft
  until their set screws are tightened, so the head height is set at assembly:
  line the motor pulley up with the 250-tooth pulley's teeth.

### Altitude

- The shaft crosses the arm 70 below its top (height ≈ 818 in the model).
- Pulley and motor move to the outer face of the arm. The pulley is the azimuth
  one without the notches: four Ø5.5 holes on Ø39 instead, and a Ø20.4 bore
  that centres it on the shaft. It lies flat on the flange of a second STHRB20
  holder on the outer end of the shaft, hub toward the bearing (about 3 clear of
  the outer bearing's insert), bolted to it with four M5 × 20 screws and nuts.
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

### Hand unit

[`cad/hand-unit.solidcraft`](../cad/hand-unit.solidcraft), STLs next to it
(`hand-unit-bottom.stl`, `hand-unit-top.stl`, `hand-unit-button.stl`, print three
buttons). The design also holds mock-ups of the breadboard, the joystick and the
switches, which are not printed.

- Electronics on a solderable 400-point breadboard
  ([tebl Breadboard 400](https://github.com/tebl/breadboard/tree/main/Breadboard%20400):
  81.28 × 53.34, two Ø3 holes on the centre line, 66.04 apart).
- Gamepad shape for two hands, 125.5 × 87.2 × 32.7: a body round the breadboard
  and two grips towards the user. Walls 2.5. Top edge rounded R6, bottom edge R4.
  A bottom tray (13 tall) and a top shell, joined by four M3 screws from below
  (heads in Ø6.5 counterbores) into M3 heat-set inserts (Ø4 holes) in columns of
  the top shell: two beside the back corners of the breadboard, two in the grips.
- Breadboard on two posts 5 tall, its two holes on M3 inserts: underside 7.5 above
  the outer bottom face. The breadboard's height is set by the posts, not by the
  soldering.
- Joystick (KY-023 module, PCB 26.5 × 34.2 measured, four holes taken as 27 × 20)
  screwed under the top shell on four bosses with M3 inserts, so the thumb force
  goes into the case, not into solder joints, and its height does not depend on
  the breadboard. The module lies above the right end of the breadboard, its 34.2
  side front to back, its PCB 6 above the breadboard; it connects by five wires.
  Stick axis 36 right of and 3 in front of the breadboard centre (`joy_x`,
  `joy_y`); Ø24 opening in the top, which clears the corners of the stick's 16.6
  square frame. The top face is 16 above the module PCB (`joy_top_h`); the top of
  the stick is about 30 above it, so 14 above the top face.
- Three 12 × 12 tactile switches on the breadboard, each across the centre gap
  (pins in rows 6.35 either side of the centre line, 5.08 apart), centred on
  columns 7, 12 and 17 of 30 from the left (x = −21.59, −8.89, 3.81 from the
  breadboard centre), pressed flat on the board. Printed buttons, Ø9, in Ø9.5
  guide tubes under the top, 3 above the top face. Each rests on its switch's
  actuator (`sw_h` = 7.3 above the breadboard) and a Ø11.5 flange under the tube
  keeps it in. Switches of another height need `sw_h` changed and the buttons
  printed again.
- One 3 mm LED, on 3.3 V and GND through a resistor (for example 4.7 kΩ for a dim
  red light), as a power indicator: the 8-wire cable has no free wire for an LED
  driven by the firmware. It sits in a Ø3.1 hole in a boss under the top (5.5
  thick in all), above the middle button, 18 behind it: pushed in from inside up
  to its rim and glued, on two wires, so the length of its legs does not matter.
- Cable: Ø6.5 hole in the back wall, half in each shell, so it clamps the cable
  when the case is closed. A cable tie inside takes the pull.
- Printed with no supports: the bottom on its floor, the top on its top face, the
  buttons on their flange.

### Printed parts

- Printed: pulleys, motor spacers, azimuth pulley ring, battery cradle,
  electronics case, hand-unit case and buttons, drilling templates, cable guides.
- The motors are not carried by printed parts in bending: each one is bolted to an
  aluminium plate, and the printed spacer between the plate and the L is clamped by
  bolts, so it mostly works in compression. Belt pull is about 10–60 N; a printed
  bracket flexing 0.05 at the motor would turn the axis by about 2′ each time the
  torque reverses.
- Files: [`cad/pulley-gt2-250t.solidcraft`](../cad/pulley-gt2-250t.solidcraft)
  (azimuth) and [`cad/pulley-gt2-250t-altitude.solidcraft`](../cad/pulley-gt2-250t-altitude.solidcraft),
  each with its STL (three bodies that overlap by 0.1, fused by the slicer; about 145 g
  of PETG, no supports), [`cad/azimuth-motor-spacer.stl`](../cad/azimuth-motor-spacer.stl),
  [`cad/altitude-motor-spacer.stl`](../cad/altitude-motor-spacer.stl),
  [`cad/azimuth-pulley-ring.stl`](../cad/azimuth-pulley-ring.stl). The spacers
  take three M3 heat-set inserts (Ø5 outside, in Ø4.4 × 7 holes) for the plate,
  with M3 × 8 screws. Their four Ø6.6 holes to the extrusion have Ø13 × 2.5
  counterbores for an M6 head and a DIN 125 washer, which leaves 7.5 of
  plastic, so an M6 × 20 screw reaches about 6 into a slot-10 T-nut.
- The azimuth pulley's centre (Ø28.5 bore, four Ø9.5 notches on Ø39) follows the
  STHRB20 table from the seller; check it against the real holder before
  printing. Its four Ø6.6 holes on the 48 × 48 square are for the M6 screws.
  The altitude pulley has a Ø20.4 bore and four Ø5.5 holes on Ø39 instead, and
  no M6 holes. Six curved slots (16 wide, on R58) lighten the web.
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
- The plate's brackets are held by T-nuts in the stem's slots, so the axis can be
  moved by sliding them along the stem.

## Transport

Three pieces:

| Piece | Joint | Size in the model |
| --- | --- | --- |
| Base (T with feet, plate, ring and azimuth pulley) | the shaft holder's M5 screws, through the pulley's notches | ≈ 65 × 38 × 12 cm |
| Mount head (L with bearings, shafts, motors, belts, electronics case, battery, clasp) | — | ≈ 77 × 49 × 17 cm |
| Telescope (tube) | the knob of the tube clasp | ≈ 90 × 15 × 17 cm |

- The bearings stay assembled, so the preload is kept. The azimuth shaft and its
  holder come off with the head.
- The azimuth pulley stays bolted to the base. Before lifting the head, slide the
  azimuth motor spacer toward the axis and lift the belt off the 250-tooth
  pulley; it stays on the motor pulley. The altitude belt keeps its tension.
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
| M8 bolts, DIN 9021 washers, 45-series T-nuts (bearings) | 4 | 4 | €5 per bag | — |
| Closed GT2 belt 610, 6 wide | 1 | 1 | — | existing |
| GT2 pulley 20 teeth, 5 bore | 1 | 1 | — | existing |
| NEMA 17 17HS4401S | 1 | 1 | — | existing |
| GT2 pulley 250 teeth, motor spacer (PETG) | 1 | 1 | filament | printed |
| Pulley ring (PETG) | – | 1 | filament | printed |
| NEMA 17 motor plate, 3 mm aluminium | 1 | 1 | — | existing (Motedis) |
| M3 heat-set inserts (Ø5 outside) and M3 × 8 screws (plate to spacer), M3 × 8 screws (motor to plate) | 3 + 4 | 3 + 4 | €5 | — |
| M6 × 20 screws (DIN 912), DIN 125 washers, slot-10 M6 T-nuts (spacer to L) | 4 | 4 | €4 | — |
| M5 × 20 screws, nuts and DIN 9021 washers, pulley to holder | 4 | – | €1 | — |
| M5 × 20 socket head screws, holder to plate (two with hammer-head T-nuts) | – | 4 | — | existing |
| M6 × 22 screws and DIN 125 washers, pulley and ring to plate | – | 4 | — | — |
| M5 × 20 screws cut to 16, holder to clasp plate | 4 | – | — | — |
| Retaining compound (Loctite 638 or 648), thread-locker | 1 | – | €10 | — |
| Aluminium plate 100 × 121 × 8 for the tube clasp | 1 | – | not checked | — |

The battery is listed in [architecture](architecture.md#power), the electronics in
its [parts list](architecture.md#parts-list).

## 3D model

[`3d/index.html`](3d/index.html) shows the redesigned mount: camera presets, a
see-through mode for the drilled holes, an exploded view, sliders for azimuth and
altitude, and a "pack for transport" layout with the size of each piece. Clicking a
part shows what it is and whether it is bought, printed or existing.

Geometry is simplified (profiles, bearings, shaft holders, pulleys, motors, belts,
battery, case and tube as basic shapes). Fasteners other than the bearing bolts are
not drawn, and neither are the motor cables.
