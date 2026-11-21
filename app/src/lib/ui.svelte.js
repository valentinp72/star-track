// App-wide messages: a short toast, and an alert that waits for OK.

class Ui {
  toast = $state(null); // text
  alert = $state(null); // {title, text}
  #timer = null;

  showToast(text, ms = 3000) {
    this.toast = text;
    clearTimeout(this.#timer);
    this.#timer = setTimeout(() => (this.toast = null), ms);
  }

  showAlert(title, text) {
    this.alert = { title, text };
  }
}

export const ui = new Ui();
