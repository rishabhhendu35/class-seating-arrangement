const form = document.getElementById("planner");
const output = document.getElementById("output");
const statusLine = document.getElementById("status");

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  statusLine.textContent = "C is generating your classroom layout…";

  const formData = new URLSearchParams(new FormData(form));

  try {
    const response = await fetch("/generate", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: formData.toString()
    });

    if (!response.ok) throw new Error("The C server returned an error.");
    const html = await response.text();
    output.innerHTML = html;
    statusLine.textContent = "Seating plan generated successfully by the C program.";
  } catch (error) {
    statusLine.textContent = "Could not contact the C server. Make sure classroom_server.exe is running.";
    console.error(error);
  }
});
