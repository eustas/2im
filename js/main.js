document.addEventListener('DOMContentLoaded', function() {
  var content = /** @type{!Node} */ (document.getElementById("content"));
  var input = /** @type {!HTMLInputElement} */ (document.getElementById('encoded'));

  /** @param{string} text */
  function report(text) {
    var p = document.createElement("p");
    var t = document.createTextNode(text);
    p.appendChild(t);
    content.appendChild(p);
  }

  /**
   * @param{string} str
   * @return{number}
   */
  function chr(str) {
    return str.codePointAt(0);
  }

  function decodeInput() {
    var materialized = input.value;
    var dematerialized = Array.from(materialized).map(chr);
    var decoded = cjkbytes.decode(dematerialized);
    report(decoded.join(", "));
  }

  /** @return{!Array<number>} */
  function doEncode() {
    var e = new range.OptimalEncoder();
    var maxValue = 16;
    for (var i = 0; i < 256000; ++i) {
      //var value = (i * i + i) % maxValue;
      //var value = Math.floor(maxValue * Math.random());
      var value = 15;
      e.encodeRange(value, value + 1, maxValue);
    }
    return e.finish();
  }

  /**
   * @param{!Array<number>} data
   * @return{number}
   */
  function doDecode(data) {
    var maxValue = 16;
    var bad = 0;
    var d = new range.Decoder(data);
    for (var i = 0; i < 256000; ++i) {
      var value = d.currentCount(maxValue);
      //var expected = (i * i + i) % maxValue;
      var expected = 15;
      if (value != expected) {
        bad++;
      }
      d.removeRange(value, value + 1);
    }
    return bad;
  }

  function measure() {
    var a = (new Date()).getTime();
    var res = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    //for (var j = 0; j < 10000; ++j) {
      var enc = doEncode();
      var bad = doDecode(enc);
      var c = enc.length;
      c = c - 127999;
      res[c] = 1 + (res[c] | 0);
    //}
    var b = (new Date()).getTime();
    report((b - a) + ":" + res.join(", ") + " # " + bad);
  }

  measure();
  input.addEventListener("input", decodeInput);
});
