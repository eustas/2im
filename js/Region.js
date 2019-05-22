goog.provide('twim.Region');

/**
 * @constructor
 */
twim.Region.DistanceRange = function() {
  /** @type{number} */
  this.min = 0;
  /** @type{number} */
  this.max = 0;
  /** @type{number} */
  this.numLines = 0;
  /** @type{number} */
  this.lineQuant = 0;
};

/**
 * @param{!Int32Array} region
 * @param{number} angle
 * @param{number} lineQuant
 * @return {void}
 */
twim.Region.DistanceRange.prototype.update = function(region, angle, lineQuant) {
  let /** @type{number} */ count3 = region[region.length - 1] * 3;
  if (count3 == 0) {
    throw "empty region";
  }

  // Note: SinCos.SIN is all positive -> d0 <= d1
  let /** @type{number} */ nx = twim.SinCos.SIN[angle];
  let /** @type{number} */ ny = twim.SinCos.COS[angle];
  let /** @type{number} */ mi = 2147483647;
  let /** @type{number} */ ma = -2147483648;
  for (let /** @type{number} */ i = 0; i < count3; i += 3) {
    let /** @type{number} */ y = region[i];
    let /** @type{number} */ x0 = region[i + 1];
    let /** @type{number} */ x1 = region[i + 2] - 1;
    let /** @type{number} */ d0 = ny * y + nx * x0;
    let /** @type{number} */ d1 = ny * y + nx * x1;
    mi = mi < d0 ? mi : d0;
    ma = ma > d1 ? ma : d1;
  }
  this.min = mi;
  this.max = ma;
  this.numLines = ((ma - mi) / lineQuant) | 0;
  this.lineQuant = lineQuant;
};

/**
 * @param{number} line
 * @return {number}
 */
twim.Region.DistanceRange.prototype.distance = function(line) {
  if (this.numLines > 1) {
    return this.min + (((this.max - this.min) - (this.numLines - 1) * this.lineQuant) / 2) | 0 + this.lineQuant * line;
  } else {
    return ((this.max + this.min) / 2) | 0;
  }
};


/**
 * @param{!Int32Array} region
 * @param{number} angle
 * @param{number} d
 * @param{!Int32Array} left
 * @param{!Int32Array} right
 * @return {void}
 */
twim.Region.splitLine = function(region, angle, d, left, right) {
  // x * nx + y * ny >= d
  let /** @type{number} */ nx = twim.SinCos.SIN[angle];
  let /** @type{number} */ ny = twim.SinCos.COS[angle];
  let /** @type{number} */ count3 = region[region.length - 1] * 3;
  let /** @type{number} */ leftCount = 0;
  let /** @type{number} */ rightCount = 0;
  if (nx == 0) {
    // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2];
      if (y * ny >= d) {
        left[leftCount] = y;
        left[leftCount + 1] = x0;
        left[leftCount + 2] = x1;
        leftCount += 3;
      } else {
        right[rightCount] = y;
        right[rightCount + 1] = x0;
        right[rightCount + 2] = x1;
        rightCount += 3;
      }
    }
  } else {
    // nx > 0 -> x ?? (d - y * ny) / nx
    d = 2 * d + nx;
    ny = 2 * ny;
    nx = 2 * nx;
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x = ((d - y * ny) / nx) | 0;
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2];
      if (x < x1) {
        left[leftCount] = y;
        left[leftCount + 1] = Math.max(x, x0);
        left[leftCount + 2] = x1;
        leftCount += 3;
      }
      if (x > x0) {
        right[rightCount] = y;
        right[rightCount + 1] = x0;
        right[rightCount + 2] = Math.min(x, x1);
        rightCount += 3;
      }
    }
  }
  left[left.length - 1] = leftCount / 3;
  right[right.length - 1] = rightCount / 3;
};
