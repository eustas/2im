document.addEventListener('DOMContentLoaded', function() {
  let content = /** @type{!Node} */ (document.getElementById("content"));
  let input = /** @type {!HTMLInputElement} */ (document.getElementById('encoded'));

  /** @param{string} text */
  function report(text) {
    let p = document.createElement("p");
    let t = document.createTextNode(text);
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
    let materialized = input.value;
    let dematerialized = Array.from(materialized).map(chr);
    let decoded = CjkDecoder.decode(dematerialized);
    report(decoded.join(", "));
  }

  input.addEventListener("input", decodeInput);
});
