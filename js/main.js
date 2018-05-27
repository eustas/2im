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

  function doTest() {
    var e = new range.OptimalEncoder();
    for (var i = 0; i < data.length; i += 3) {
      e.encodeRange(data[i], data[i + 1], data[i + 2]);
    }
    var actual = e.finish();
    var d = new range.Decoder(actual);
    for (var i = 0; i < data.length; i += 3) {
      var value = d.currentCount(data[i + 2]);
      if ((value < data[i]) || (value >= data[i + 1])) {
        report(value + "!" + data[i] + "-" + data[i + 1] + ":" + data[i + 2]);
        return;
      }
      d.removeRange(data[i], data[i + 1]);
    }
    for (var j = 0; j < expected.length; ++j) {
      if (expected[j] != actual[j]) {
        report(j + "/" + expected.length + " " + actual.length);
        return;
      }
    }
  }
/*
var expected = [
102, 70, 200, 135, 239, 117, 17, 3, 195, 97, 159, 84, 183, 139, 185, 97, 24, 90, 5, 2, 132, 60, 129, 176, 86, 130, 222, 4, 8, 88, 
28, 97, 92, 28, 54, 171, 159, 183, 229, 217, 186, 183, 207, 18, 240, 229, 27, 201, 109, 77, 188, 203, 154, 231, 18, 175, 199, 82, 185, 25, 
97, 202, 6, 174, 110, 165, 149, 23, 30, 162, 144, 218, 108, 143, 145, 109, 153, 202, 69, 64, 218, 16, 172, 42, 171, 237, 64, 50, 76, 234, 
149, 43, 16, 156, 93, 90, 6, 108, 55, 25, 70, 172, 234, 199, 249, 19, 198, 233, 166, 95, 22, 197, 178, 103, 131, 113, 247, 63, 92, 226, 
227, 91, 231, 168, 71, 31, 111, 152, 111, 231, 163, 113, 96, 178, 86, 144, 104, 83, 160, 150, 8, 225, 51, 238, 89, 65, 67, 2, 86, 150, 
64, 180, 114, 67, 69, 59, 115, 73, 21, 91, 104, 89, 186, 156, 177, 209, 168, 10, 109, 58, 137, 96, 61, 198, 123, 67, 38, 209, 119, 41, 
145, 27, 223, 190, 59, 114, 10, 124, 117, 95, 208, 67, 18, 230, 120, 82, 51, 255, 90, 211, 146, 249, 212, 86, 114, 4, 196, 138, 46, 183, 
75, 42, 223, 109, 72, 230, 111, 186, 61, 60, 211, 204, 95, 248, 255, 153, 237, 220, 178, 45, 217, 103, 206, 52, 21, 195, 54, 41, 75, 251, 
86, 124, 95, 255, 16, 0, 207, 221, 67, 1, 254, 161, 93, 80, 239, 200, 146, 118, 116, 110, 101, 206, 55, 202, 154, 217, 190, 223, 187, 232, 
105, 179, 79, 131, 240, 13, 246, 144, 113, 110, 233, 9, 147, 197, 124, 188, 229, 75, 18, 9, 99, 107, 151, 69, 221, 170, 102, 246, 101, 32, 
205, 178, 153, 187, 143, 157, 247, 28, 44, 168, 99, 46, 180, 158, 48, 110, 73, 48, 14, 21, 77, 16, 68, 3, 214, 23, 230, 188, 0, 223, 
176, 12, 64, 235, 62, 22, 82, 159, 239, 61, 105, 178, 10, 7, 35, 78, 167, 154, 50, 181, 255, 229, 103, 216, 190, 15
];
*/
  doTest();
  input.addEventListener("input", decodeInput);
});
