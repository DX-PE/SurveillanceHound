<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Follow Scent

Open **Log**, select a sighting and tap **Follow Scent**. Selection waits for a new, valid reading from the same address, address type and radio. Live reception is at most five seconds old; after five seconds it shows the last reading and waits for a repeat, and after 15 seconds it shows **Signal lost**. The 30-second graph uses one latest reading per second, leaves gaps between sparse readings, and fades as data ages out. Trend compares recent samples; the dog reacts to the current reception level. Walls, antennas, channel hopping and advertisement intervals affect RSSI, so neither the graph nor the dog gives direction or exact distance.

**Back to Hound** keeps the selected scent; tap **Follow** to reopen it. **Stop following** clears it without stopping scanning. Stop/Start scanning keeps the target but clears readings, waiting for a fresh sample after resuming. Selecting a new scent or changing demo mode resets the follow session; reboot clears it. A supported Samsung broadcast ID keeps the selection across MAC changes. A changed broadcast ID, or other address rotation, requires a new selection. A disabled detector suspends updates for its selected category. Follow readings bypass alert throttling but do not independently add log entries, discoveries or meals.

In the browser, send a synthetic sighting first, select it through the device Log, and use **Follow a signal** in the surrounding controls. Change reception, send one reading or enable **Repeat reading every second**. Disable repeat to test waiting/lost states. These readings are synthetic and do not use the USB board.


## Following a deauth scent

Follow Scent charts reception from the selected transmitter identity. Ordinary management frames from that address can update the graph; a steady reading is not proof of an ongoing deauthentication burst. The [deauth detector](DETECTIONS.md) separately needs 20 deauthentication/disassociation frames from one source/channel within 10 seconds.

Check fresh Log evidence and its last-seen time to see whether the detector emitted another event. Independent passive capture on the reported channel and access-point logs are needed to distinguish valid management traffic, retransmissions and possible abuse. A source MAC or a rate threshold does not establish who sent a frame or why.
