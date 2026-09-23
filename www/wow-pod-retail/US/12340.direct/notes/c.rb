require 'github/markup'

input_file = 'Patch_000001.md'
output_file = 'Patch_000001.html'

# 3. Read and clean the file content
raw_markdown = File.read(input_file)

# 4. Convert Markdown to raw HTML snippet
raw_html = GitHub::Markup.render(input_file, raw_markdown)

# 5. Wrap the HTML in a full document using GitHub's CSS CDN
full_html_document = <<~HTML
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Rendered Markdown</title>
    
    <!-- GitHub Markdown CSS stylesheet -->
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/github-markdown-css/5.5.1/github-markdown.min.css">
    
    <style>
      /* Make body a flexbox container filling the viewport height */
      body {
        display: flex;
        justify-content: center; /* Horizontally center */
        align-items: center;     /* Vertically center */
        min-height: 100vh;
        margin: 0;
        padding: 20px;
        box-sizing: border-box;
        background-color: #0d1117; /* Matches GitHub dark background; adapts via theme */
      }

      /* Constrain content container */
      .markdown-body {
        box-sizing: border-box;
        width: 100%;
        max-width: 980px;
        padding: 45px;
        border-radius: 6px;
      }

      @media (max-width: 767px) {
        .markdown-body {
          padding: 15px;
        }
      }

      .markdown-body ul {
        list-style-type: disc !important;
        padding-left: 2em !important;
      }

      .markdown-body ol {
        list-style-type: decimal !important;
        padding-left: 2em !important;
      }

      /* Sub-lists get open circles */
      .markdown-body ul ul {
        list-style-type: circle !important;
      }
    </style>
  </head>
  <body>
    <!-- The class "markdown-body" activates GitHub's stylesheet -->
    <article class="markdown-body">
      #{raw_html}
    </article>
  </body>
  </html>
HTML

# 6. Write output to file
File.write(output_file, full_html_document)
puts "Generated #{output_file} with GitHub theme styling!"
