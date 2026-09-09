function dmax = calcdmax(B, x)
%CALCDMAX Maximum F2 hop length dmax of eq. (5), km (uncapped).
%   dmax = p533.calcdmax(B, x)  with B, x from p533.calcB. Section 3.5.1.1
%   caps this at 4000 km for the basic MUF; the caller applies the cap.
dmax = 4780 + (12610 + 2140 / x ^ 2 - 49720 / x ^ 4 + 688900 / x ^ 6) * (1 / B - 0.303);
end
