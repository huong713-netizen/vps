import express from 'express';
import fetch from 'node-fetch';
import { load } from 'cheerio';

const app = express();
const PORT = process.env.PORT || 4000;

app.use(express.urlencoded({ extended: true }));

app.get('/', async (req, res) => {
  const url = req.query.url;
  if (!url) {
    return res.status(400).send('Vui lòng gửi tham số url, ví dụ: /?url=https://example.com');
  }

  let targetUrl;
  try {
    targetUrl = new URL(url);
  } catch {
    return res.status(400).send('URL không hợp lệ');
  }

  try {
    const response = await fetch(targetUrl.href, {
      headers: {
        'User-Agent': 'SimpleProxy/1.0',
      }
    });

    if (!response.ok) {
      return res.status(response.status).send(`Lỗi khi lấy trang: ${response.statusText}`);
    }

    const contentType = response.headers.get('content-type') || '';
    if (!contentType.includes('text/html')) {
      const buffer = await response.arrayBuffer();
      res.set('content-type', contentType);
      return res.send(Buffer.from(buffer));
    }

    const bodyText = await response.text();

    const $ = load(bodyText, { decodeEntities: false });

    $('a[href]').each((i, el) => {
      const href = $(el).attr('href');
      try {
        const absoluteUrl = new URL(href, targetUrl.href).href;
        $(el).attr('href', `/?url=${encodeURIComponent(absoluteUrl)}`);
      } catch {
        // Bỏ qua href không hợp lệ
      }
    });

    // Nếu muốn, bạn có thể rewrite thêm src, link, form action, script src...

    res.set('content-type', 'text/html; charset=utf-8');
    res.send($.html());
  } catch (e) {
    console.error(e);
    res.status(500).send('Lỗi server: ' + e.message);
  }
});

app.listen(PORT, () => {
  console.log(`Server chạy tại http://localhost:${PORT}`);
});
