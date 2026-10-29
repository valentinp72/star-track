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

- The shaft stands in a 20 mm rigid flange coupling bolted to the existing
  95 × 70 plate with four M6 knurled thumb screws (DIN 464), so the mount head
  lifts off without tools.
- A 20 × 32 × 14 shaft collar (Kramp "stelring", listed by a reseller as 705A20)
  under the lower bearing carries the weight.
- A printed 250-tooth pulley (the old one is broken) is clamped on the fixed shaft
  with a printed 20 mm hub. The azimuth motor moves from the base to under the far
  end of the 380 bar and turns with the L. It sits 152.4 from the axis: with a
  20-tooth motor pulley, that is the centre distance of a 610 closed GT2 belt. The battery and the electronics
  are on the L too, so no cable crosses the turning joint.
- The bearing stack is taller than the current case: in the model, standing on its
  feet, the 380 bar sits at height 223–268.

### Altitude

- The shaft crosses the arm 70 below its top (height ≈ 818 in the model).
- Pulley and motor move to the outer face of the arm. The motor sits 152.4 below the
  shaft (610 closed GT2 belt, as on azimuth) on a printed bracket with slotted holes
  for belt tension.
- On the inner face, a flange coupling on the shaft carries a 100 × 121 × 8
  aluminium plate with the original Omegon tube clasp. The coupling is turned 45°
  and two of its holes are drilled out to 10.5 so the clasp bolts (60 apart) pass
  through it. The tube can slide −60 / +40 in the clasp for balancing, and comes out
  when the clasp is opened.
- A new, smaller electronics case without a screen sits lower on the outer face
  (130 × 90 × 50 at height 360–490 in the model), because the outer bearing and
  pulley take the current case's place. The E-stop and two LEDs are on its face. The XT60 power socket and the hand-unit socket are underneath.
- The joystick and buttons are in a hand-held unit on a cable, not on the mount.
- Near the zenith the back of the tube clears the top of the azimuth shaft.

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

### Printed parts

- Printed: pulleys and hubs, motor brackets, battery cradle, electronics case,
  drilling templates, cable guides.
- Not printed: anything that carries the telescope's weight in bending (bearing
  housings, shaft supports, the link between tube and shaft). Plastic creeps under
  constant load and the play would come back.
- Material: PETG or ASA. PLA softens in the sun and creeps.
- Every printed part must fit the 180 × 180 × 180 bed of a Bambu Lab A1 mini. The
  largest is the 250-tooth pulley: Ø159.2 pitch, Ø168 over the flanges, which
  leaves about 6 on each side (no room for a wide brim). The battery cradle
  (159 long) and the electronics case (130 tall) also fit.

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
| Base (T with feet and plate) | four thumb screws on the azimuth coupling | ≈ 65 × 38 × 12 cm |
| Mount head (L with bearings, shafts, motors, belts, electronics case, battery, clasp) | — | ≈ 77 × 49 × 17 cm |
| Telescope (tube) | the knob of the tube clasp | ≈ 90 × 15 × 17 cm |

- The bearings stay assembled, so the preload is kept. The azimuth shaft, its
  coupling and the 250-tooth pulley come off with the head; the belts keep their
  tension.
- The thumb screws go into four M6 holes tapped in the 95 × 70 plate.
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
| Hardened 20 mm steel shaft, cut to 200 | 1 | 1 | €8 each | AliExpress (`20mm linear shaft`) |
| Rigid flange coupling, 20 mm | 1 | 1 | €5 each | AliExpress |
| Shaft collar 20 × 32 × 14 | – | 1 | €2.50 | Kramp |
| M8 bolts, DIN 9021 washers, 45-series T-nuts | 4 | 4 | €5 per bag | — |
| Closed GT2 belt 610, 6 wide | 1 | 1 | — | existing |
| GT2 pulley 20 teeth, 5 bore | 1 | 1 | — | existing |
| NEMA 17 17HS4401S | 1 | 1 | — | existing |
| GT2 pulley 250 teeth and hub, motor bracket (PETG) | 1 | 1 | filament | printed |
| Aluminium plate 100 × 121 × 8 for the tube clasp | 1 | – | not checked | — |
| M6 knurled thumb screws (DIN 464) | – | 4 | €4 | — |

The battery is listed in [architecture](architecture.md#power), the electronics in
its [parts list](architecture.md#parts-list).

## 3D model

[`3d/index.html`](3d/index.html) shows the redesigned mount: camera presets, a
see-through mode for the drilled holes, an exploded view, sliders for azimuth and
altitude, and a "pack for transport" layout with the size of each piece. Clicking a
part shows what it is and whether it is bought, printed or existing.

Geometry is simplified (profiles, bearings, couplings, pulleys, motors, belts,
battery, case and tube as basic shapes). Fasteners other than the bearing bolts and
thumb screws are not drawn, and neither are the motor cables.
