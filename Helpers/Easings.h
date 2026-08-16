#pragma once

//-----------------------------------------------------------------------------

// Easing curves for animations: they map linear 0..1 progress to eased 0..1
// position, clamping input outside that range
namespace easing
{
	[[ nodiscard ]] float smoothstep ( float t );		// The classic s-curve, 3t^2 - 2t^3
	[[ nodiscard ]] float smootherstep ( float t );	// Flatter ends, steeper middle, 6t^5 - 15t^4 + 10t^3
}
//-----------------------------------------------------------------------------
