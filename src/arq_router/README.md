# Arq Router

`src/arq_router` is an Arqma-owned privacy-routing scaffold.

This module intentionally avoids importing Lokinet branding into the public API.
The goal is to reserve daemon-side integration points, configuration names and
service lifecycle hooks while future implementation work stays under Arqma
copyright and product direction.

Current status:

- config key constants for future daemon wiring
- a stub `RouterService` lifecycle class
- no embedded router daemon and no transport implementation yet

Non-goals for this scaffold:

- shipping a Lokinet-branded interface
- implying production routing is already implemented
- coupling the daemon to a separate router binary before requirements exist
