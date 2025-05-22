# Introduction
This repository is a patch provided for SONiC community to support AsterFusion switch hardware.
The branch of this repository corresponds to the branch of sonic-buildimage. eg 202311 of sonic-campus-patches corresponds to 202311 of sonic-buildimage.

# How to apply patches
1. Download SONiC buildimage
   git clone https://github.com/sonic-net/sonic-buildimage.git -b 202311
2. Install the repository into the sonic-buildimage directory.
   git clone https://github.com/asterfusion/asternos_patches.git -b 202311
3. Checkout SONiC to designated commit id
   git checkout $COMMIT_ID (you can find it in the install.sh)
4. 'make init' in sonic_buildimage path
5. './install.sh' in asternos_patches path

# Supported platforms

| Vendor | device | ASIC | SKU |
| --- | --- | --- | --- |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-D1-HPW-M |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-D2-HPW-M |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-HPW-M |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-M-D1-SWP |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-M-D2-SWP |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-M-SWP |
| Asterfusion | cx102s_16gt_a1 | Marvell | CX102S-16GT-PTP-M-SWP |
| Asterfusion | cx102s_8gt_a2 | Marvell | CX102S-8GT-HPW-M |
| Asterfusion | cx102s_8gt_a2 | Marvell | CX102S-8GT-M-SWP |
| Asterfusion | cx102s_8gt_b1 | Marvell | CX102S-8MT-HPW-M |
| Asterfusion | cx104s_24gt_b | Marvell | CX104S-24GT-M-AC |
| Asterfusion | cx104s_48gt_b | Marvell | CX104S-48GT-M-AC |
| Asterfusion | cx104s_56gt_b | Marvell | CX104S-56GT-M-AC |
| Asterfusion | cx104s_64gt_b | Marvell | CX104S-64GT-M-AC |
| Asterfusion | cx202p_16s | Marvell | CX202P-16S-M |
| Asterfusion | cx202p_16s | Marvell | CX202P-16S-M-H |
| Asterfusion | cx202p_24s | Marvell | CX202P-24S-M |
| Asterfusion | cx202p_24s | Marvell | CX202P-24S-M-H |
| Asterfusion | cx202p_24y | Marvell | CX202P-24Y-M |
| Asterfusion | cx202p_24y | Marvell | CX202P-24Y-M-H |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-HPW1-M-AC |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-HPW2-M-AC |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-M-AC |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-M-S |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-M-SWP2 |
| Asterfusion | cx204y_24gt_c | Marvell | CX204Y-24GT-M-SWP4 |
| Asterfusion | cx204y_48gt_d | Marvell | CX204Y-48GT-HPW2-M-ACM |
| Asterfusion | cx204y_48gt_d | Marvell | CX204Y-48GT-M-AC |
| Asterfusion | cx204y_48gt_d | Marvell | CX204Y-48GT-M-S |
| Asterfusion | cx206p_24s | Marvell | CX206P-24S-M |
| Asterfusion | cx206p_24s | Marvell | CX206P-24S-M-H |
| Asterfusion | cx206p_48s_ptp | Marvell | CX206P-48S-PTP-M |
| Asterfusion | cx206p_48s_ptp | Marvell | CX206P-48S-PTP-M-H |
| Asterfusion | cx206p_48s | Marvell | CX206P-48S-M |
| Asterfusion | cx206p_48s | Marvell | CX206P-48S-M-H |
| Asterfusion | cx206y_48gt_b | Marvell | CX206Y-48GT-HPW4-M |
| Asterfusion | cx206y_48gt_b | Marvell | CX206Y-48GT-M |
| Asterfusion | cx206y_48gt_b | Marvell | CX206Y-48GT-M-H |
| Asterfusion | cx206y_48gt_b | Marvell | CX206Y-48GT-M-HWP4 |
| Asterfusion | cx206y_48gt_b | Marvell | CX206Y-48GT-M-HWP8 |
| Asterfusion | cx308p_48y_n | Marvell | CX308P-48Y-M_FL00E01 |
| Asterfusion | cx308p_48y_n | Marvell | CX308P-48Y-M_FL00E02 |
| Asterfusion | cx308p_48y_n | Marvell | CX308P-48Y-M_FL00E03 |
| Asterfusion | cx308p_48y_n | Marvell | CX308P-48Y-M-H_FL00E01 |
| Asterfusion | cx308p_48y_n | Marvell | CX308P-48Y-M-H_FL00E02 |
| Asterfusion | cx532p_n | Marvell | CX532P-M_FL00E02 |
| Asterfusion | cx532p_n | Marvell | CX532P-M_FL00E03 |
| Asterfusion | cx532p_n | Marvell | CX532P-M-H_FL00E02 |
| Asterfusion | cx532p_n | Marvell | CX532P-M-H_FL00E03 |
| Asterfusion | cx532p_n | Marvell | CX532P-N_FL00E02 |
| Asterfusion | cx532p_n | Marvell | CX532P-N_FL00E03 |





