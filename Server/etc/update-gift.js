import axios from "axios";
import { readFileSync, writeFileSync } from "fs";

(async () => {
    if(process.argv.length < 3) return console.log("Please input Tiktok Username");
    let tiktok = process.argv[2];

    console.log("Parsing gift for " + tiktok);
    
    let gift = await axios.get("https://indofinity.com/api/v1/gifts?username=" + tiktok);
    let ourGift = JSON.parse(readFileSync("./gifts.json", "utf8"));
    let ourGiftImage = JSON.parse(readFileSync("./gift-img.json", "utf8"));
    let giftData = gift.data.data;

    // Ambil semua ID dari API sebagai acuan
    const apiGiftIDs = giftData.map(g => g.gift_id);

    // Tambah gift baru & image jika belum ada
    giftData.forEach((element) => {
        let contex = {
            id: element.gift_id,
            name: element.name,
            diamond_count: element.diamond_count,
            run_effect: ""
        };

        if (!ourGift.find((x) => x.id === element.gift_id)) {
            console.log("🆕 New Gift: " + element.name);
            ourGift.push(contex);
        }

        if (!ourGiftImage.find((x) => x.id === element.gift_id)) {
            console.log("🖼️ New Gift Image: " + element.name);
            ourGiftImage.push({
                id: element.gift_id,
                name: element.name,
                image: element.image ?? ""
            });
        }
    });

    // Hapus gift yang tidak ada lagi di API
    let beforeGiftCount = ourGift.length;
    let beforeImageCount = ourGiftImage.length;

    ourGift = ourGift.filter(g => apiGiftIDs.includes(g.id));
    ourGiftImage = ourGiftImage.filter(g => apiGiftIDs.includes(g.id));

    let removedGiftCount = beforeGiftCount - ourGift.length;
    let removedImageCount = beforeImageCount - ourGiftImage.length;

    if (removedGiftCount > 0) {
        console.log(`Removed ${removedGiftCount} gift(s) from gifts.json`);
    }

    if (removedImageCount > 0) {
        console.log(`Removed ${removedImageCount} gift image(s) from gift-img.json`);
    }

    // Sort and write
    ourGift.sort((a, b) => a.diamond_count - b.diamond_count);

    writeFileSync("./gifts.json", JSON.stringify(ourGift, null, 4), "utf8");
    writeFileSync("./gift-img.json", JSON.stringify(ourGiftImage, null, 4), "utf8");
})();